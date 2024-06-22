#include "capture.hpp"

Capture::Capture(const std::string& device, const std::string& outFileName, bool isWriteToFile) : count(300), writeToFile(isWriteToFile)
{
    fd = open(device.c_str(), O_RDWR);
    if (fd < 0)
    {
        printf("Failed to open device\n");
    }
	std::string name = outFileName + std::string(".raw");
    outFile = open(name.c_str(), O_RDWR | O_CREAT, 0666);
    if (outFile < 0)
    {
        printf("Failed to open output.raw\n");
    }
}

Capture::~Capture()
{
	for (int i =0; i < NBUF; i++)
	{
		munmap(buffer[i], size);
	}

    close(fd);
    close(outFile);
}

int Capture::setFormat(int width, int height, int format)
{
	struct v4l2_format inner_format = {0};
	inner_format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	inner_format.fmt.pix.width = width;
	inner_format.fmt.pix.height = height;
	inner_format.fmt.pix.pixelformat = format;
	inner_format.fmt.pix.field = V4L2_FIELD_NONE;

	int res = ioctl(fd, VIDIOC_S_FMT, &inner_format);
	if(res == -1) {
		perror("Could not set format");
		exit(EXIT_FAILURE);
	}

	return res;
}

void Capture::rotateImage(int angle)
{
	struct v4l2_control control;
	memset (&control, 0, sizeof (control));
	control.id = V4L2_CID_ROTATE;
	control.value = angle;


	int res = ioctl(fd, VIDIOC_S_CTRL, &control);
	if(res == -1) {
		perror("Could not set format");
		exit(EXIT_FAILURE);
	}
	
}

void Capture::setFrameRate(enum v4l2_buf_type const type ,unsigned int framerate)
{
	struct v4l2_fract timeperframe = { 1, framerate };
		int rc;
	struct v4l2_streamparm parm;
	memset(&(parm), 0, sizeof(parm));
	parm.type = type;
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
	if (rc == -1)
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

void Capture::queryCapabilites()
{
	struct v4l2_capability cap;

	if (-1 == ioctl(fd, VIDIOC_QUERYCAP, &cap)) {
		perror("Query capabilites");
		exit(EXIT_FAILURE);
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		fprintf(stderr, "Device is no video capture device\\n");
		exit(EXIT_FAILURE);
	}


	if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
		fprintf(stderr, "Device does not support read i/o\\n");
	}


	if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
		fprintf(stderr, "Devices does not support streaming i/o\\n");
		exit(EXIT_FAILURE);
	}
}

int Capture::requestBuffer()
{

	struct v4l2_requestbuffers req = {0};
	req.count = NBUF;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	if (ioctl(fd, VIDIOC_REQBUFS, &req) == -1) {
		perror("Requesting Buffer");
		exit(EXIT_FAILURE);
	}


	if (req.count > NBUF)
	{
		fprintf(stderr, "Increase NBUF to at least %i\n", nbufs);
		exit(EXIT_FAILURE);
	}
    
    for (int i = 0; i < NBUF; i++)
	{
		size = queryBuffer(i, &buffer[i]);

		queueBuffer(i);

	}

	return size;
}


int Capture::queryBuffer(int index, unsigned char **buffer)
{
	struct v4l2_buffer buf = {0};

	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	buf.index = index;

	if(ioctl(fd, VIDIOC_QUERYBUF, &buf) == -1) {
		perror("Could not query buffer");
		exit(EXIT_FAILURE);
	}

	*buffer = (u_int8_t*)mmap (NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
	return buf.length;
}

int Capture::queueBuffer(int index)
{
	struct v4l2_buffer bufd = {0};
	bufd.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	bufd.memory = V4L2_MEMORY_MMAP;
	bufd.index = index;

	if (ioctl(fd, VIDIOC_QBUF, &bufd) == -1) {
		perror("Queue Buffer");
		exit(EXIT_FAILURE);
	}

	return bufd.bytesused;
}



int Capture::startStreaming()
{
	unsigned int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if(ioctl(fd, VIDIOC_STREAMON, &type) == -1)
        {
		perror("VIDIOC_STREAMON");
		exit(EXIT_FAILURE);
	}

	return 0;
}

int Capture::dequeueBuffer()
{
	struct v4l2_buffer bufd = {0};
	bufd.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	bufd.memory = V4L2_MEMORY_MMAP;
	bufd.index = 0;

	if (ioctl(fd, VIDIOC_DQBUF, &bufd) == -1)
        {
		perror("DeQueue Buffer");
		exit(EXIT_FAILURE);
	}

	return bufd.index;
}


int Capture::stopStreaming()
{
	unsigned int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if(ioctl(fd, VIDIOC_STREAMOFF, &type) == -1)
        {
		perror("VIDIOC_STREAMON");
		exit(EXIT_FAILURE);
	}
	return 0;
}

int Capture::readFrame()
{
	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	struct timeval tv = {0};
	tv.tv_sec = 2;
	int r = select(fd+1, &fds, NULL, NULL, &tv);

	if(-1 == r)
	{
		perror("Waiting for Frame");
		exit(1);
	}

	/* Step 7: Dequeue buffer */
	index = dequeueBuffer();

	printf("Start %p  length %d index %d", buffer[index], size, index);

	if(writeToFile)
	{
		ssize_t bytesSize =  write(outFile, buffer[index], size);
		if (bytesSize <= 0)
        {
            printf("Cant write bytes to file %lu", bytesSize);
        }
	}

	return index;

	// buf.index = index;
	// buf.length = size;
	// buf.start = buffer[index];


}

void Capture::attachBuffers(unsigned char **buf)
{
	buffer = buf;
}
