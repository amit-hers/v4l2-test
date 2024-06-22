
#include "encoder.hpp"

Encoder::Encoder(const std::string& outputFileName, int port, std::string& client) : outputFormat({0}), inputFormat({0})
{
    fd = open("/dev/video11", O_RDWR);
    if (fd < 0)
    {
        printf("Failed to open /dev/video11\n");
    }
    std::string name = std::string("encode") + outputFileName + std::string(".h264");
    outFile = open(name.c_str(), O_RDWR | O_CREAT, 0666);
    if (outFile < 0)
    {
        printf("Failed to open encode.h264 n");
    }
    socket = openSocket();
    initAddr(adderess, port, client);

    memset(&inputBuffer, 0, sizeof(VideoBuffer));
    memset(&outputBuffer, 0, sizeof(VideoBuffer));
}

Encoder::~Encoder()
{
    destroyBuffer(&inputBuffer);
    destroyBuffer(&outputBuffer);
    
    closeSocket(socket);
    close(fd);
    close(outFile);
}


void Encoder::setFrameRate(enum v4l2_buf_type const type ,unsigned int framerate)
{
	struct v4l2_fract timeperframe = { 1, framerate };
		int rc;
	struct v4l2_streamparm parm = {
		.type = type
	};

	struct v4l2_fract *tpf;
	uint32_t *cap;

	switch (type) {
		case V4L2_BUF_TYPE_VIDEO_CAPTURE:
			tpf = &parm.parm.capture.timeperframe;
			cap = &parm.parm.capture.capability;
			break;
		case V4L2_BUF_TYPE_VIDEO_OUTPUT:
			tpf = &parm.parm.output.timeperframe;
			cap = &parm.parm.output.capability;
			break;
		default:
			printf("Unsupported buffer type: %u\n", type);
	}

	printf("Setup framerate for to %.1f\n",
			(float)timeperframe.denominator / timeperframe.numerator);

	rc = ioctl(fd, VIDIOC_G_PARM, &parm);
	if (rc != 0)
		printf("Failed to get device streaming parameters\n");

	if (!(*cap & V4L2_CAP_TIMEPERFRAME)) {
		printf("Device does not support framerate adjustment");
		return;
	}
	printf("--------> tbf %d/%d <----------------",tpf->denominator, tpf->numerator);
	*tpf = timeperframe;

	rc = ioctl(fd, VIDIOC_S_PARM, &parm);
	if (rc != 0)
		printf("Failed to set device streaming parameters");

	if (tpf->numerator != timeperframe.numerator ||
	    tpf->denominator != timeperframe.denominator) {
		printf("Setup framerate for to %.1f\n",(float)timeperframe.denominator / timeperframe.numerator);
		timeperframe = *tpf;
	}
}

bool Encoder::setupInputBuffer(int width, int height, unsigned int format)
{
    // Setup "input" format

    inputFormat.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    inputFormat.fmt.pix_mp.width = width;
    inputFormat.fmt.pix_mp.height = height;
    inputFormat.fmt.pix_mp.pixelformat = format;
    inputFormat.fmt.pix_mp.field = V4L2_FIELD_ANY;
    inputFormat.fmt.pix_mp.colorspace = V4L2_COLORSPACE_DEFAULT;
    inputFormat.fmt.pix_mp.quantization = V4L2_QUANTIZATION_FULL_RANGE;
    inputFormat.fmt.pix_mp.ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
    inputFormat.fmt.pix_mp.num_planes = 1;

    if (ioctl(fd, VIDIOC_S_FMT, &inputFormat) < 0) {
        printf("Failed to set input format\n");
        return false;
    }

    return true;
}



bool Encoder::setupOutputBuffer(int width, int height)
{
    
    // Setup output format
    outputFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    outputFormat.fmt.pix_mp.width = width;
    outputFormat.fmt.pix_mp.height = height;
    outputFormat.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_H264;
    outputFormat.fmt.pix_mp.field = V4L2_FIELD_ANY;
    outputFormat.fmt.pix_mp.colorspace = V4L2_COLORSPACE_DEFAULT;
    outputFormat.fmt.pix_mp.quantization = V4L2_QUANTIZATION_FULL_RANGE;
    outputFormat.fmt.pix_mp.xfer_func = V4L2_XFER_FUNC_DEFAULT;
    outputFormat.fmt.pix_mp.ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
    outputFormat.fmt.pix_mp.num_planes = 1;
    outputFormat.fmt.pix_mp.plane_fmt[0].sizeimage = (1024 + 512) << 10;

    int err = ioctl(fd, VIDIOC_S_FMT, &outputFormat);
    if (err < 0) {
        printf("Failed to set output format %d\n", err);
        return false;
    }

    return  initBuffer(V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, &inputBuffer) && initBuffer(V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, &outputBuffer); 
}

bool Encoder::setEncoderOption(unsigned int id, int value)
{
    struct v4l2_control control = {
        .id = id,
        .value = value
    };

    if (ioctl(fd, VIDIOC_S_CTRL, &control) < 0) {
        printf("Can't set encoder option id: %d value: %d\n", id, value);
        return false;
    }

    return true;
}

bool Encoder::mapBuffer(v4l2_buf_type type, struct VideoBuffer *buffer)
{
    struct v4l2_buffer* inner = &buffer->inner;

    memset(inner, 0, sizeof(*inner));
    inner->type = type;
    inner->memory = V4L2_MEMORY_MMAP;
    inner->index = 0;
    inner->length = 1;
    inner->m.planes = &buffer->plane;

    if (ioctl(fd, VIDIOC_QUERYBUF, inner) < 0)
    {
        printf("Can't queue video buffer with type %d\n", type);
        return false;
    }

    buffer->length = inner->m.planes[0].length;
        printf("!!!!!!!!!!!!!!! The inner plane %d !!!!!!!!!!\n",  buffer->length);

    buffer->start = mmap(nullptr, buffer->length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, inner->m.planes[0].m.mem_offset);


    if (ioctl(fd, VIDIOC_QBUF, inner))
    {
        printf("Can't queue video buffer with type %d\n", type);
        return false;
    }

    return true;
}

bool Encoder::initBuffer(enum v4l2_buf_type type, struct VideoBuffer* buffer)
{
    struct v4l2_requestbuffers request = {0};
    request.count = 1;
    request.type = type;
    request.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_REQBUFS, &request) < 0) {
        printf("Failed to init buffer\n");
        return false;
    }

    if (request.count < 1) {
        printf("Failed to allocate buffer memory, type %d, count: %d\n", type, request.count);
        return false;
    }

    if (!mapBuffer(type, buffer))
        return false;

    return true;
}

bool Encoder::encodeFrame(unsigned char * buf, int size) {
    struct v4l2_buffer input_buf = {0};
    struct v4l2_plane input_plane = {0};
    input_buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    input_buf.length = 1;
    input_buf.m.planes = &input_plane;
    input_buf.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_DQBUF, &input_buf) < 0)
    {
        printf("Failed to dequeue input buffer\n");
        return false;
    }

    memcpy(inputBuffer.start, buf, size);

    if (ioctl(fd, VIDIOC_QBUF, &input_buf) < 0) {
        printf("failed to queue input buffer\n");
        return false;
    }

    struct v4l2_buffer output_buf = {0};
    struct v4l2_plane output_plane = {0};
    output_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    output_buf.length = 1;
    output_buf.m.planes = &output_plane;
    output_buf.memory = V4L2_MEMORY_MMAP;

    // Lock the output encoder buffer
    if (ioctl(fd, VIDIOC_DQBUF, &output_buf) < 0) {
        printf("failed to dequeue output buffer\n");
        return false;
    }

    write(outFile, outputBuffer.start, output_buf.m.planes[0].bytesused);

    udpSend(socket, adderess, outputBuffer.start, output_buf.m.planes[0].bytesused);

    if (ioctl(fd, VIDIOC_QBUF, &output_buf) < 0) {
        printf("failed to queue output buffer\n");
        return false;
    }

    return true;
}

bool Encoder::destroyBuffer(VideoBuffer *buffer)
{
    if (munmap(buffer->start, buffer->length) < 0) {
        printf("Failed to unmap buffer %d\n", buffer->inner.type);
        return false;
    }

    *buffer = VideoBuffer();

    return true;
}

void Encoder::debug()
{
    printf("Input buffer length: %i bytes, stride: %i, width: %i height %i\n", 
    inputBuffer.length, inputFormat.fmt.pix_mp.plane_fmt[0].bytesperline, inputFormat.fmt.pix_mp.width, inputFormat.fmt.pix_mp.height);

}

bool Encoder::startStream()
{
    if (ioctl(fd, VIDIOC_STREAMON, &inputBuffer.inner.type) < 0) {
        printf("Can't set VIDIOC_STREAMON for input buffer\n");
        return false;
    }

    if (ioctl(fd, VIDIOC_STREAMON, &outputBuffer.inner.type) < 0) {
        printf("Can't set VIDIOC_STREAMON for output buffer\n");
        return false;
    }

    return true;
}


bool Encoder::stopStream()
{
    if (ioctl(fd, VIDIOC_STREAMOFF, &inputBuffer.inner.type) < 0) {
        printf("Can't set VIDIOC_STREAMON for input buffer\n");
        return false;
    }

    if (ioctl(fd, VIDIOC_STREAMOFF, &outputBuffer.inner.type) < 0) {
        printf("Can't set VIDIOC_STREAMON for output buffer\n");
        return false;
    }

    return true;
}