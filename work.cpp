#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <linux/videodev2.h>
#include <linux/v4l2-controls.h>
#include <cstring>
#include <errno.h>
#include <cstdlib>

struct VideoBuffer {
    void* start;
    int length;

    struct v4l2_buffer inner;
    struct v4l2_plane plane;
};

int fd_encoder;
FILE *outputFile;
struct v4l2_format inputFormat = {0};
struct v4l2_format outputFormat = {0};
// Allocate buffers

VideoBuffer inputBuffer;
VideoBuffer outputBuffer;


bool setEncoderOption(int fd, unsigned int id, int value)
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

bool mapBuffer(int fd, v4l2_buf_type type, VideoBuffer *buffer)
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

bool initBuffer(int fd, enum v4l2_buf_type type, struct VideoBuffer* buffer)
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

    if (!mapBuffer(fd, type, buffer))
        return false;

    return true;
}

bool encodeFrame(int fd, VideoBuffer& inputBuffer, VideoBuffer& outputBuffer, void* srcData, int file, int size) {
    struct v4l2_buffer input_buf = {0};
    struct v4l2_plane input_plane = {0};
    input_buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    input_buf.length = 1;
    input_buf.m.planes = &input_plane;
    input_buf.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_DQBUF, &input_buf) < 0) {
        printf("Failed to dequeue input buffer\n");
        return false;
    }

    static time_t now = 1705597574;
    now++; // Dummy increase our "timestamp"

    struct timeval ts = {
        .tv_sec = now / 1000000,
        .tv_usec = now % 1000000,
    };

    memcpy(inputBuffer.start, srcData, size);
    //memset(inputBuffer.start, 218, inputBuffer.length); // Fill the whole buffer with gray (218, 218, 218) color

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

    write(file, outputBuffer.start, output_buf.m.planes[0].bytesused);

    if (ioctl(fd, VIDIOC_QBUF, &output_buf) < 0) {
        printf("failed to queue output buffer\n");
        return false;
    }

    return true;
}

bool destroyBuffer(int fd, VideoBuffer *buffer)
{
    if (munmap(buffer->start, buffer->length) < 0) {
        printf("Failed to unmap buffer %d\n", buffer->inner.type);
        return false;
    }

    *buffer = VideoBuffer();

    return true;
}



#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <unistd.h>

#include <errno.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>


#define NBUF 3


void query_capabilites(int fd)
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


int set_format(int fd)
{
	struct v4l2_format format = {0};
	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	format.fmt.pix.width = 320;
	format.fmt.pix.height = 240;
	format.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;
	format.fmt.pix.field = V4L2_FIELD_NONE;

	int res = ioctl(fd, VIDIOC_S_FMT, &format);
	if(res == -1) {
		perror("Could not set format");
		exit(EXIT_FAILURE);
	}

	return res;
}


int request_buffer(int fd, int count)

{

	struct v4l2_requestbuffers req = {0};

	req.count = count;

	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	req.memory = V4L2_MEMORY_MMAP;


	if (ioctl(fd, VIDIOC_REQBUFS, &req) == -1) {

		perror("Requesting Buffer");

		exit(EXIT_FAILURE);

	}


	return req.count;

}


int query_buffer(int fd, int index, unsigned char **buffer)
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


int queue_buffer(int fd, int index)
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


int start_streaming(int fd)
{
	unsigned int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if(ioctl(fd, VIDIOC_STREAMON, &type) == -1)
        {
		perror("VIDIOC_STREAMON");
		exit(EXIT_FAILURE);
	}

	return 0;
}


int dequeue_buffer(int fd)
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


int stop_streaming(int fd)
{
	unsigned int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if(ioctl(fd, VIDIOC_STREAMOFF, &type) == -1)
        {
		perror("VIDIOC_STREAMON");
		exit(EXIT_FAILURE);
	}
	return 0;
}


int mainloop()

{

	unsigned char *buffer[NBUF];
	int fd = open("/dev/video0", O_RDWR);
	int size;
	int index;
	int nbufs;
        int count = 300;

	fd_set fds;
	int file = open("output.raw", O_RDWR | O_CREAT, 0666);
	int e_file = open("encode.h264", O_RDWR | O_CREAT, 0666);

	/* Step 1: Query capabilities */
	query_capabilites(fd);

	/* Step 2: Set format */
	set_format(fd);

	/* Step 3: Request Format */
	nbufs = request_buffer(fd, NBUF);

	if ( nbufs > NBUF)
	{
		fprintf(stderr, "Increase NBUF to at least %i\n", nbufs);
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i < NBUF; i++)
	{
		/* Step 4: Query buffers 

		 * Assume all sizes is equal.

		 * */

		size = query_buffer(fd, i, &buffer[i]);


		/* Step 5: Queue buffer */

		queue_buffer(fd, i);

	}


	/* Step 6: Start streaming */

	start_streaming(fd);

	for(int i=0; i<count; i++)
	{
		printf("capture frame %d\n",i);
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
		index = dequeue_buffer(fd);

		write(file, buffer[index], size);
		/* Step 8: Stop streaming */

                encodeFrame(fd_encoder, inputBuffer, outputBuffer, buffer[index], e_file, size);

		queue_buffer(fd, index);

	}

	stop_streaming(fd);
	/* Cleanup the resources */

	for (int i =0; i < NBUF; i++)
	{
		munmap(buffer[i], size);
	}

	close(file);
	close(fd);
        close(e_file);
	return 0;
}


























int main(int argc, char **argv)
{

    fd_encoder = open("/dev/video11", O_RDWR);
    if (fd_encoder < 0)
    {
        printf("Failed to open /dev/video11\n");
        return 1;
    }

    // Write the output frame data to the output file

    char str[80];
    memset(str, 0, 80);

    sprintf(str, "output_%i_%i.h264", 320, 240);

    outputFile = fopen(str, "wba\n");
    if (!outputFile) {
        perror("Error opening output file\n");
        return 1;
    }


    setEncoderOption(fd_encoder, V4L2_CID_MPEG_VIDEO_BITRATE, 1200000);
    setEncoderOption(fd_encoder, V4L2_CID_MPEG_VIDEO_H264_I_PERIOD, 30);
    setEncoderOption(fd_encoder, V4L2_CID_MPEG_VIDEO_H264_PROFILE, V4L2_MPEG_VIDEO_H264_PROFILE_CONSTRAINED_BASELINE);
    setEncoderOption(fd_encoder, V4L2_CID_MPEG_VIDEO_H264_LEVEL, V4L2_MPEG_VIDEO_H264_LEVEL_4_0);
    setEncoderOption(fd_encoder, V4L2_CID_MPEG_VIDEO_REPEAT_SEQ_HEADER, 1);
    setEncoderOption(fd_encoder, V4L2_CID_MPEG_VIDEO_H264_MIN_QP, 2);
    setEncoderOption(fd_encoder, V4L2_CID_MPEG_VIDEO_H264_MAX_QP, 8);

    // Setup "input" format

    inputFormat.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    inputFormat.fmt.pix_mp.width = 320;
    inputFormat.fmt.pix_mp.height = 240;
    inputFormat.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12;
    inputFormat.fmt.pix_mp.field = V4L2_FIELD_ANY;
    inputFormat.fmt.pix_mp.colorspace = V4L2_COLORSPACE_DEFAULT;
    inputFormat.fmt.pix_mp.quantization = V4L2_QUANTIZATION_FULL_RANGE;
    inputFormat.fmt.pix_mp.ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
    inputFormat.fmt.pix_mp.num_planes = 1;

    if (ioctl(fd_encoder, VIDIOC_S_FMT, &inputFormat) < 0) {
        printf("Failed to set input format\n");
        return 1;
    }

    // Setup output format
    outputFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    outputFormat.fmt.pix_mp.width = 320;
    outputFormat.fmt.pix_mp.height = 240;
    outputFormat.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_H264;
    outputFormat.fmt.pix_mp.field = V4L2_FIELD_ANY;
    outputFormat.fmt.pix_mp.colorspace = V4L2_COLORSPACE_DEFAULT;
    outputFormat.fmt.pix_mp.quantization = V4L2_QUANTIZATION_FULL_RANGE;
    outputFormat.fmt.pix_mp.xfer_func = V4L2_XFER_FUNC_DEFAULT;
    outputFormat.fmt.pix_mp.ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
    outputFormat.fmt.pix_mp.num_planes = 1;
    outputFormat.fmt.pix_mp.plane_fmt[0].sizeimage = (1024 + 512) << 10;

    if (ioctl(fd_encoder, VIDIOC_S_FMT, &outputFormat) < 0) {
        printf("Failed to set output format\n");
        return 1;
    }

    initBuffer(fd_encoder, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE, &inputBuffer);
    initBuffer(fd_encoder, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, &outputBuffer);

    printf("Input buffer length: %i bytes, stride: %i, width: %i height %i\n", 
        inputBuffer.length, inputFormat.fmt.pix_mp.plane_fmt[0].bytesperline, inputFormat.fmt.pix_mp.width, inputFormat.fmt.pix_mp.height);



    if (ioctl(fd_encoder, VIDIOC_STREAMON, &inputBuffer.inner.type) < 0) {
        printf("Can't set VIDIOC_STREAMON for input buffer\n");
        return 1;
    }

    if (ioctl(fd_encoder, VIDIOC_STREAMON, &outputBuffer.inner.type) < 0) {
        printf("Can't set VIDIOC_STREAMON for output buffer\n");
        return 1;
    }


    mainloop();

    if (ioctl(fd_encoder, VIDIOC_STREAMOFF, &inputBuffer.inner.type) < 0) {
        printf("Can't set VIDIOC_STREAMON for input buffer\n");
        return 1;
    }

    if (ioctl(fd_encoder, VIDIOC_STREAMOFF, &outputBuffer.inner.type) < 0) {
        printf("Can't set VIDIOC_STREAMON for output buffer\n");
        return 1;
    }

    destroyBuffer(fd_encoder, &inputBuffer);
    destroyBuffer(fd_encoder, &outputBuffer);

    fclose(outputFile);
    close(fd_encoder);

    fprintf(stderr, "\n");
        return 0;
}