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

bool encodeFrame(int fd, VideoBuffer& inputBuffer, VideoBuffer& outputBuffer, void* srcData, FILE* outputFile) {
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

    // memcpy(inputBuffer.start, srcData, inputBuffer.length);
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

    fwrite(outputBuffer.start, 1, output_buf.m.planes[0].bytesused, outputFile);

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

int runTest(int width, int height)
{







    // void* srcData = malloc(inputBuffer.length);
    // memset(srcData, 128, inputBuffer.length); // Fill the whole frame with gray color

    // int yStride = inputFormat.fmt.pix_mp.plane_fmt[0].bytesperline;
    // int uvStride = inputFormat.fmt.pix_mp.plane_fmt[0].bytesperline / 2;
    // int h = inputFormat.fmt.pix_mp.height;

    // unsigned char* yData = (unsigned char*)srcData;
    // unsigned char* uData = yData + yStride * h;
    // unsigned char* vData = uData + uvStride * h / 2;

    // unsigned int planeYsize = yStride * h;
    // unsigned int planeUsize = uvStride * h / 2;
    // unsigned int planeVsize = uvStride * h / 2;

    // printf("plane Y (offset %u, size %u), plane U (offset %u, size %u), plane V (offset %u, size %u), total (input buffer length %u, calculated length %u)\n", 
    //         0, planeYsize, 
    //         planeYsize, planeUsize, 
    //         planeYsize + planeUsize, planeVsize,
    //         inputBuffer.length, planeYsize + planeUsize + planeVsize);

    // // Rectangle coords
    // int rectX = 0; 
    // int rectY = 0;
    // int rectW = 640;
    // int rectH = 480;

    // // RGB blue (39, 115, 255) -> YUV (111, 222, 77)

    // // Y plane has double size comparable to UV
    // for (int y = rectY * 2; y < rectY * 2 + rectH * 2; y++)
    // {
    //     memset((yData + rectX * 2) + (y * yStride), 111, rectW * 2); // Fill the Y plane
    // }

    // for (int y = rectY; y < rectY + rectH; y++)
    // {
    //     memset((uData + rectX) + (y * uvStride), 222, rectW); // Fill the U plane
    //     memset((vData + rectX) + (y * uvStride), 77, rectW); // Fill the V plane
    // }

    encodeFrame(fd_encoder, inputBuffer, outputBuffer, NULL, outputFile);
    // encodeFrame(fd_encoder, inputBuffer, outputBuffer, srcData, outputFile);
    // encodeFrame(fd_encoder, inputBuffer, outputBuffer, srcData, outputFile);
    // encodeFrame(fd_encoder, inputBuffer, outputBuffer, srcData, outputFile);
    // encodeFrame(fd_encoder, inputBuffer, outputBuffer, srcData, outputFile);

    // free(srcData);

    // Shoutdown the stream

    // if (ioctl(fd_encoder, VIDIOC_STREAMOFF, &inputBuffer.inner.type) < 0) {
    //     printf("Can't set VIDIOC_STREAMON for input buffer\n");
    //     return 1;
    // }

    // if (ioctl(fd_encoder, VIDIOC_STREAMOFF, &outputBuffer.inner.type) < 0) {
    //     printf("Can't set VIDIOC_STREAMON for output buffer\n");
    //     return 1;
    // }

    return 0;
}







































/*
 *  V4L2 M2M video example
 *
 *  This program can be used and distributed without restrictions.
 *
 * This program is based on the examples provided with the V4L2 API
 * see https://linuxtv.org/docs.php for more information
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <getopt.h>             /* getopt_long() */

#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>

#define CLEAR(x) memset(&(x), 0, sizeof(x))

enum io_method {
        IO_METHOD_READ,
        IO_METHOD_MMAP,
        IO_METHOD_USERPTR,
};

struct buffer {
        void   *start;
        size_t  length;
};

static char            *dev_name;
static enum io_method   io = IO_METHOD_MMAP;
static int              fd = -1;
struct buffer          *buffers;
struct buffer          *buffers_out;
static unsigned int     n_buffers;
static unsigned int     n_buffers_out;
static int              m2m_enabled;
static char            *out_filename;
static FILE            *out_fp;
static int              force_format;
static int              frame_count = 70;
static char            *in_filename;
static FILE            *in_fp;

static void errno_exit(const char *s)
{
        fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
        exit(EXIT_FAILURE);
}

static int xioctl(int fh, int request, void *arg)
{
        int r;

        do {
                r = ioctl(fh, request, arg);
        } while (-1 == r && EINTR == errno);

        return r;
}

static void process_image(const void *p, int size)
{
        if (!out_fp)
                out_fp = fopen("out_filename", "wb");
        if (out_fp)
                fwrite(p, size, 1, out_fp);
        

        memcpy(inputBuffer.start, p, size);

        encodeFrame(fd_encoder, inputBuffer, outputBuffer, NULL, outputFile);


        fflush(stderr);
        fprintf(stderr, ".");
}

static void supply_input(void *buf, unsigned int buf_len, unsigned int *bytesused)
{
    unsigned char *buf_char = (unsigned char*)buf;
    if (in_fp) {
        *bytesused = fread(buf, 1, buf_len, in_fp);
        if (*bytesused != buf_len)
            fprintf(stderr, "Short read %u instead of %u\n", *bytesused, buf_len);
        else
            fprintf(stderr, "Read %u bytes. First 4 bytes %02x %02x %02x %02x\n", *bytesused, 
                buf_char[0], buf_char[1], buf_char[2], buf_char[3]);
    }
}

static int read_frame(enum v4l2_buf_type type, struct buffer *bufs, unsigned int n_buffers)
{
        struct v4l2_buffer buf;
        unsigned int i;

        switch (io) {
        case IO_METHOD_READ:
                if (-1 == read(fd, bufs[0].start, bufs[0].length)) {
                        switch (errno) {
                        case EAGAIN:
                                return 0;

                        case EIO:
                                /* Could ignore EIO, see spec. */

                                /* fall through */

                        default:
                                errno_exit("read");
                        }
                }

                process_image(bufs[0].start, bufs[0].length);
                break;

        case IO_METHOD_MMAP:
                CLEAR(buf);

                buf.type = type;
                buf.memory = V4L2_MEMORY_MMAP;

                if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
                        switch (errno) {
                        case EAGAIN:
                                return 0;

                        case EIO:
                                /* Could ignore EIO, see spec. */

                                /* fall through */

                        default:
                                errno_exit("VIDIOC_DQBUF");
                        }
                }

                assert(buf.index < n_buffers);

                if (type == V4L2_BUF_TYPE_VIDEO_CAPTURE)
                {
                    printf("Data\n");
                    process_image(bufs[buf.index].start, buf.bytesused);
                }
                else
                    supply_input(bufs[buf.index].start, bufs[buf.index].length, &buf.bytesused);

                if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                        errno_exit("VIDIOC_QBUF");
                break;

        case IO_METHOD_USERPTR:
                CLEAR(buf);

                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_USERPTR;

                if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
                        switch (errno) {
                        case EAGAIN:
                                return 0;

                        case EIO:
                                /* Could ignore EIO, see spec. */

                                /* fall through */

                        default:
                                errno_exit("VIDIOC_DQBUF");
                        }
                }

                for (i = 0; i < n_buffers; ++i)
                        if (buf.m.userptr == (unsigned long)bufs[i].start
                            && buf.length == bufs[i].length)
                                break;

                assert(i < n_buffers);

                process_image((void *)buf.m.userptr, buf.bytesused);

                if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                        errno_exit("VIDIOC_QBUF");
                break;
        }

        return 1;
}

static void stop_capture(enum v4l2_buf_type type)
{
    if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
        errno_exit("VIDIOC_STREAMOFF");
}

static void stop_capturing(void)
{

        switch (io) {
        case IO_METHOD_READ:
                /* Nothing to do. */
                break;

        case IO_METHOD_MMAP:
        case IO_METHOD_USERPTR:
                stop_capture(V4L2_BUF_TYPE_VIDEO_CAPTURE);

                if (m2m_enabled) 
                    stop_capture(V4L2_BUF_TYPE_VIDEO_OUTPUT);
                break;
        }
}

static void start_capturing_mmap(enum v4l2_buf_type type, struct buffer *bufs, unsigned int n_bufs)
{
    unsigned int i;

    for (i = 0; i < n_bufs; ++i) {
        struct v4l2_buffer buf;

        CLEAR(buf);
        buf.type = type;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (type == V4L2_BUF_TYPE_VIDEO_OUTPUT)
            supply_input(bufs[i].start, bufs[i].length, &buf.bytesused);

        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                errno_exit("VIDIOC_QBUF");
    }
    
    if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
            errno_exit("VIDIOC_STREAMON");
}

static void start_capturing(void)
{
        unsigned int i;
        enum v4l2_buf_type type;

        switch (io) {
        case IO_METHOD_READ:
                /* Nothing to do. */
                break;

        case IO_METHOD_MMAP:
                start_capturing_mmap(V4L2_BUF_TYPE_VIDEO_CAPTURE, buffers, n_buffers);
                if (m2m_enabled)
                    start_capturing_mmap(V4L2_BUF_TYPE_VIDEO_OUTPUT, buffers_out, n_buffers_out);
                break;

        case IO_METHOD_USERPTR:
                for (i = 0; i < n_buffers; ++i) {
                        struct v4l2_buffer buf;

                        CLEAR(buf);
                        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                        buf.memory = V4L2_MEMORY_USERPTR;
                        buf.index = i;
                        buf.m.userptr = (unsigned long)buffers[i].start;
                        buf.length = buffers[i].length;

                        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                                errno_exit("VIDIOC_QBUF");
                }
                type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
                        errno_exit("VIDIOC_STREAMON");
                break;
        }
}

static void unmap_buffers(struct buffer *buf, unsigned int n)
{
    unsigned int i;
    for (i = 0; i < n; ++i)
            if (-1 == munmap(buf[i].start, buf[i].length))
                    errno_exit("munmap");
}

static void free_buffers_mmap(enum v4l2_buf_type type)
{
    struct v4l2_requestbuffers req;

    CLEAR(req);

    req.count = 0;
    req.type = type;
    req.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
            if (EINVAL == errno) {
                    fprintf(stderr, "%s does not support "
                             "memory mappingn", dev_name);
                    exit(EXIT_FAILURE);
            } else {
                    errno_exit("VIDIOC_REQBUFS");
            }
    }
}

static void uninit_device(void)
{
        unsigned int i;

        switch (io) {   
        case IO_METHOD_READ:
                free(buffers[0].start);
                break;

        case IO_METHOD_MMAP:
                unmap_buffers(buffers, n_buffers);
                free_buffers_mmap(V4L2_BUF_TYPE_VIDEO_CAPTURE);
                if (m2m_enabled) {
                    unmap_buffers(buffers_out, n_buffers_out);
                    free_buffers_mmap(V4L2_BUF_TYPE_VIDEO_OUTPUT);
                }
                break;

        case IO_METHOD_USERPTR:
                for (i = 0; i < n_buffers; ++i)
                        free(buffers[i].start);
                break;
        }

        free(buffers);
}

static void init_read(unsigned int buffer_size)
{
        buffers = (buffer*)calloc(1, sizeof(*buffers));

        if (!buffers) {
                fprintf(stderr, "Out of memory\n");
                exit(EXIT_FAILURE);
        }

        buffers[0].length = buffer_size;
        buffers[0].start = malloc(buffer_size);

        if (!buffers[0].start) {
                fprintf(stderr, "Out of memory\n");
                exit(EXIT_FAILURE);
        }
}

static void init_mmap(enum v4l2_buf_type type, struct buffer **bufs_out, unsigned int *n_bufs)
{
        struct v4l2_requestbuffers req;
        struct buffer *bufs;
        unsigned int n;

        CLEAR(req);

        req.count = 4;
        req.type = type;
        req.memory = V4L2_MEMORY_MMAP;

        if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
                if (EINVAL == errno) {
                        fprintf(stderr, "%s does not support "
                                 "memory mappingn", dev_name);
                        exit(EXIT_FAILURE);
                } else {
                        errno_exit("VIDIOC_REQBUFS");
                }
        }

        if (req.count < 2) {
                fprintf(stderr, "Insufficient buffer memory on %s\n",
                         dev_name);
                exit(EXIT_FAILURE);
        }

        bufs = (buffer*)calloc(req.count, sizeof(*bufs));

        if (!bufs) {
                fprintf(stderr, "Out of memory\n");
                exit(EXIT_FAILURE);
        }

        for (n = 0; n < req.count; ++n) {
                struct v4l2_buffer buf;

                CLEAR(buf);

                buf.type        = type;
                buf.memory      = V4L2_MEMORY_MMAP;
                buf.index       = n;

                if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
                        errno_exit("VIDIOC_QUERYBUF");

                fprintf(stderr, "Mapping buffer %u, len %u\n", n, buf.length);
                bufs[n].length = buf.length;
                bufs[n].start =
                        mmap(NULL /* start anywhere */,
                              buf.length,
                              PROT_READ | PROT_WRITE /* required */,
                              MAP_SHARED /* recommended */,
                              fd, buf.m.offset);

                if (MAP_FAILED == bufs[n].start)
                        errno_exit("mmap");
        }
        *n_bufs = n;
        *bufs_out = bufs;
}

static void init_userp(unsigned int buffer_size)
{
        struct v4l2_requestbuffers req;

        CLEAR(req);

        req.count  = 4;
        req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_USERPTR;

        if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
                if (EINVAL == errno) {
                        fprintf(stderr, "%s does not support "
                                 "user pointer i/on", dev_name);
                        exit(EXIT_FAILURE);
                } else {
                        errno_exit("VIDIOC_REQBUFS");
                }
        }

        buffers = (buffer*) calloc(4, sizeof(*buffers));

        if (!buffers) {
                fprintf(stderr, "Out of memory\n");
                exit(EXIT_FAILURE);
        }

        for (n_buffers = 0; n_buffers < 4; ++n_buffers) {
                buffers[n_buffers].length = buffer_size;
                buffers[n_buffers].start = malloc(buffer_size);

                if (!buffers[n_buffers].start) {
                        fprintf(stderr, "Out of memory\n");
                        exit(EXIT_FAILURE);
                }
        }
}

static void init_device_out(void)
{
        struct v4l2_cropcap cropcap;
        struct v4l2_crop crop;
        struct v4l2_format fmt;
        unsigned int min;

        CLEAR(cropcap);

        cropcap.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

        if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
                crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                crop.c = cropcap.defrect; /* reset to default */

                if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop)) {
                        switch (errno) {
                        case EINVAL:
                                /* Cropping not supported. */
                                break;
                        default:
                                /* Errors ignored. */
                                break;
                        }
                }
        } else {
                /* Errors ignored. */
        }


        CLEAR(fmt);

        fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
        if (-1 == xioctl(fd, VIDIOC_G_FMT, &fmt))
                errno_exit("VIDIOC_G_FMT");
        if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_H264 && force_format) {
                fmt.fmt.pix.width       = 640;
                fmt.fmt.pix.height      = 480;
                fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;
                fmt.fmt.pix.field       = V4L2_FIELD_NONE;

                if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
                        errno_exit("VIDIOC_S_FMT");

                /* Note VIDIOC_S_FMT may change width and height. */
        }

        /* Buggy driver paranoia. */
        min = fmt.fmt.pix.width * 2;
        if (fmt.fmt.pix.bytesperline < min)
                fmt.fmt.pix.bytesperline = min;
        min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
        if (fmt.fmt.pix.sizeimage < min)
                fmt.fmt.pix.sizeimage = min;

        switch (io) {
        case IO_METHOD_READ:
                init_read(fmt.fmt.pix.sizeimage);
                break;

        case IO_METHOD_MMAP:
                init_mmap(V4L2_BUF_TYPE_VIDEO_OUTPUT, &buffers_out, &n_buffers_out);
                break;

        case IO_METHOD_USERPTR:
                init_userp(fmt.fmt.pix.sizeimage);
                break;
        }

        struct v4l2_event_subscription sub;

        memset(&sub, 0, sizeof(sub));

        sub.type = V4L2_EVENT_EOS;
        ioctl(fd, VIDIOC_SUBSCRIBE_EVENT, &sub);

        sub.type = V4L2_EVENT_SOURCE_CHANGE;
        ioctl(fd, VIDIOC_SUBSCRIBE_EVENT, &sub);
}

static void init_device(void)
{
        struct v4l2_capability cap;
        struct v4l2_cropcap cropcap;
        struct v4l2_crop crop;
        struct v4l2_format fmt;
        unsigned int min;

        if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
                if (EINVAL == errno) {
                        fprintf(stderr, "%s is no V4L2 device\n",
                                 dev_name);
                        exit(EXIT_FAILURE);
                } else {
                        errno_exit("VIDIOC_QUERYCAP");
                }
        }

        fprintf(stderr, "caps returned %04x\n", cap.capabilities);
        if (!(cap.capabilities & (V4L2_CAP_VIDEO_M2M|V4L2_CAP_VIDEO_CAPTURE))) {
                fprintf(stderr, "%s is no video capture device\n",
                         dev_name);
                exit(EXIT_FAILURE);
        }

        switch (io) {
        case IO_METHOD_READ:
                if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
                        fprintf(stderr, "%s does not support read i/o\n",
                                 dev_name);
                        exit(EXIT_FAILURE);
                }
                break;

        case IO_METHOD_MMAP:
        case IO_METHOD_USERPTR:
                if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
                        fprintf(stderr, "%s does not support streaming i/o\n",
                                 dev_name);
                        exit(EXIT_FAILURE);
                }
                break;
        }


        /* Select video input, video standard and tune here. */


        CLEAR(cropcap);

        cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
                crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                crop.c = cropcap.defrect; /* reset to default */

                if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop)) {
                        switch (errno) {
                        case EINVAL:
                                /* Cropping not supported. */
                                break;
                        default:
                                /* Errors ignored. */
                                break;
                        }
                }
        } else {
                /* Errors ignored. */
        }


        CLEAR(fmt);

        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (-1 == xioctl(fd, VIDIOC_G_FMT, &fmt))
                errno_exit("VIDIOC_G_FMT");
        if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_H264 && force_format) {
                fmt.fmt.pix.width       = 640;
                fmt.fmt.pix.height      = 480;
                fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;
                fmt.fmt.pix.field       = V4L2_FIELD_NONE;

                if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
                        errno_exit("VIDIOC_S_FMT");

                /* Note VIDIOC_S_FMT may change width and height. */
        }

        /* Buggy driver paranoia. */
        min = fmt.fmt.pix.width * 2;
        if (fmt.fmt.pix.bytesperline < min)
                fmt.fmt.pix.bytesperline = min;
        min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
        if (fmt.fmt.pix.sizeimage < min)
                fmt.fmt.pix.sizeimage = min;

        switch (io) {
        case IO_METHOD_READ:
                init_read(fmt.fmt.pix.sizeimage);
                break;

        case IO_METHOD_MMAP:
                init_mmap(V4L2_BUF_TYPE_VIDEO_CAPTURE, &buffers, &n_buffers);
                break;

        case IO_METHOD_USERPTR:
                init_userp(fmt.fmt.pix.sizeimage);
                break;
        }
        if (cap.capabilities & V4L2_CAP_VIDEO_M2M) {
            init_device_out();
            m2m_enabled = 1;
            if (in_filename) {
                in_fp = fopen(in_filename, "rb");
                if (!in_fp)
                    fprintf(stderr, "Failed to open input file %s", in_filename);
            }
        }
}

static void close_device(void)
{
        if (-1 == close(fd))
                errno_exit("close");

        fd = -1;
}

static void open_device(void)
{
        struct stat st;

        if (-1 == stat(dev_name, &st)) {
                fprintf(stderr, "Cannot identify '%s': %d, %s\n",
                         dev_name, errno, strerror(errno));
                exit(EXIT_FAILURE);
        }

        if (!S_ISCHR(st.st_mode)) {
                fprintf(stderr, "%s is no devicen", dev_name);
                exit(EXIT_FAILURE);
        }

        fd = open(dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

        if (-1 == fd) {
                fprintf(stderr, "Cannot open '%s': %d, %s\n",
                         dev_name, errno, strerror(errno));
                exit(EXIT_FAILURE);
        }
}

static void handle_event(void)
{
        struct v4l2_event ev;

        while (!ioctl(fd, VIDIOC_DQEVENT, &ev)) {
            switch (ev.type) {
            case V4L2_EVENT_SOURCE_CHANGE:
                fprintf(stderr, "Source changed\n");

                stop_capture(V4L2_BUF_TYPE_VIDEO_CAPTURE);
                unmap_buffers(buffers, n_buffers);

                fprintf(stderr, "Unmapped all buffers\n");
                free_buffers_mmap(V4L2_BUF_TYPE_VIDEO_CAPTURE);

                init_mmap(V4L2_BUF_TYPE_VIDEO_CAPTURE, &buffers, &n_buffers);

                start_capturing_mmap(V4L2_BUF_TYPE_VIDEO_CAPTURE, buffers, n_buffers);
                break;
            case V4L2_EVENT_EOS:
                fprintf(stderr, "EOS\n");
                break;
            }
        }

}

static void mainloop(void)
{
        unsigned int count;

        count = 100;

        while (count-- > 0) {
                for (;;) {
                        fd_set fds[3];
                        fd_set *rd_fds = &fds[0]; /* for capture */
                        fd_set *ex_fds = &fds[1]; /* for capture */
                        fd_set *wr_fds = &fds[2]; /* for output */
                        struct timeval tv;
                        int r;

                        if (rd_fds) {
                            FD_ZERO(rd_fds);
                            FD_SET(fd, rd_fds);
                        }

                        if (ex_fds) {
                            FD_ZERO(ex_fds);
                            FD_SET(fd, ex_fds);
                        }

                        if (wr_fds) {
                            FD_ZERO(wr_fds);
                            FD_SET(fd, wr_fds);
                        }

                        /* Timeout. */
                        tv.tv_sec = 10;
                        tv.tv_usec = 0;

                        r = select(fd + 1, rd_fds, wr_fds, ex_fds, &tv);

                        if (-1 == r) {
                                if (EINTR == errno)
                                        continue;
                                errno_exit("select");
                        }

                        if (0 == r) {
                                fprintf(stderr, "select timeout\n");
                                exit(EXIT_FAILURE);
                        }

                        if (rd_fds && FD_ISSET(fd, rd_fds)) {
                            fprintf(stderr, "Reading\n");
                            if (read_frame(V4L2_BUF_TYPE_VIDEO_CAPTURE, buffers, n_buffers))
                                break;
                        }
                        if (wr_fds && FD_ISSET(fd, wr_fds)) {
                            fprintf(stderr, "Writing\n");
                            if (read_frame(V4L2_BUF_TYPE_VIDEO_OUTPUT, buffers_out, n_buffers_out))
                                break;
                        }
                        if (ex_fds && FD_ISSET(fd, ex_fds)) {
                            fprintf(stderr, "Exception\n");
                            handle_event();
                        }
                        /* EAGAIN - continue select loop. */
                }
        }
}



int main(int argc, char **argv)
{
    dev_name = "/dev/video1";

    open_device();

    fd_encoder = open("/dev/video11", O_RDWR);
    if (fd < 0)
    {
        printf("Failed to open /dev/video11\n");
        return 1;
    }

    // Write the output frame data to the output file

    char str[80];
    memset(str, 0, 80);

    sprintf(str, "output_%i_%i.h264", 640, 480);

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
    inputFormat.fmt.pix_mp.width = 640;
    inputFormat.fmt.pix_mp.height = 480;
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
    outputFormat.fmt.pix_mp.width = 640;
    outputFormat.fmt.pix_mp.height = 480;
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


    init_device();
    start_capturing();
    mainloop();
    stop_capturing();
    uninit_device();
    close_device();


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
