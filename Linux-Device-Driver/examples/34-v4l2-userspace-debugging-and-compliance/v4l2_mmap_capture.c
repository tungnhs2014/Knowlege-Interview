// SPDX-License-Identifier: MIT
/*
 * Learning-only V4L2 MMAP capture tool.
 *
 * This program intentionally supports only single-planar YUYV capture so the
 * ioctl sequence, buffer ownership, and cleanup rules stay visible.
 */

#include <errno.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define DEFAULT_DEV "/dev/video0"
#define DEFAULT_OUT "/tmp/v4l2-capture.yuyv"
#define DEFAULT_WIDTH 640
#define DEFAULT_HEIGHT 480
#define DEFAULT_COUNT 5
#define BUFFER_COUNT 4

struct capture_buffer {
        void *start;
        size_t length;
};

static int xioctl(int fd, unsigned long request, void *arg)
{
        int ret;

        do {
                ret = ioctl(fd, request, arg);
        } while (ret == -1 && errno == EINTR);

        return ret;
}

static void usage(const char *prog)
{
        fprintf(stderr,
                "Usage: %s [device] [output] [frame-count] [width] [height]\n"
                "Default: %s %s %s %d %d %d\n",
                prog, prog, DEFAULT_DEV, DEFAULT_OUT, DEFAULT_COUNT,
                DEFAULT_WIDTH, DEFAULT_HEIGHT);
}

static void print_fourcc(unsigned int fourcc, char out[5])
{
        out[0] = fourcc & 0xff;
        out[1] = (fourcc >> 8) & 0xff;
        out[2] = (fourcc >> 16) & 0xff;
        out[3] = (fourcc >> 24) & 0xff;
        out[4] = '\0';
}

static int queue_buffer(int fd, unsigned int index)
{
        struct v4l2_buffer buf;

        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = index;

        if (xioctl(fd, VIDIOC_QBUF, &buf) == -1) {
                perror("VIDIOC_QBUF");
                return -1;
        }

        return 0;
}

int main(int argc, char **argv)
{
        const char *dev_name = DEFAULT_DEV;
        const char *out_name = DEFAULT_OUT;
        unsigned int width = DEFAULT_WIDTH;
        unsigned int height = DEFAULT_HEIGHT;
        unsigned int frame_count = DEFAULT_COUNT;
        struct capture_buffer buffers[BUFFER_COUNT];
        unsigned int mapped = 0;
        int streaming = 0;
        int fd = -1;
        int out_fd = -1;
        int ret = EXIT_FAILURE;

        memset(buffers, 0, sizeof(buffers));

        if (argc > 6) {
                usage(argv[0]);
                return EXIT_FAILURE;
        }
        if (argc > 1)
                dev_name = argv[1];
        if (argc > 2)
                out_name = argv[2];
        if (argc > 3)
                frame_count = (unsigned int)strtoul(argv[3], NULL, 0);
        if (argc > 4)
                width = (unsigned int)strtoul(argv[4], NULL, 0);
        if (argc > 5)
                height = (unsigned int)strtoul(argv[5], NULL, 0);

        fd = open(dev_name, O_RDWR | O_NONBLOCK);
        if (fd == -1) {
                perror("open video device");
                goto cleanup;
        }

        out_fd = open(out_name, O_CREAT | O_TRUNC | O_WRONLY, 0644);
        if (out_fd == -1) {
                perror("open output file");
                goto cleanup;
        }

        struct v4l2_capability cap;
        memset(&cap, 0, sizeof(cap));
        if (xioctl(fd, VIDIOC_QUERYCAP, &cap) == -1) {
                perror("VIDIOC_QUERYCAP");
                goto cleanup;
        }

        unsigned int caps = cap.capabilities;
        if (cap.capabilities & V4L2_CAP_DEVICE_CAPS)
                caps = cap.device_caps;

        if (!(caps & V4L2_CAP_VIDEO_CAPTURE)) {
                fprintf(stderr, "%s is not a single-planar capture node\n",
                        dev_name);
                goto cleanup;
        }
        if (!(caps & V4L2_CAP_STREAMING)) {
                fprintf(stderr, "%s does not support streaming I/O\n",
                        dev_name);
                goto cleanup;
        }

        struct v4l2_format fmt;
        memset(&fmt, 0, sizeof(fmt));
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.width = width;
        fmt.fmt.pix.height = height;
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
        fmt.fmt.pix.field = V4L2_FIELD_NONE;

        if (xioctl(fd, VIDIOC_S_FMT, &fmt) == -1) {
                perror("VIDIOC_S_FMT");
                goto cleanup;
        }

        char fourcc[5];
        print_fourcc(fmt.fmt.pix.pixelformat, fourcc);
        printf("Granted format: %ux%u %.4s bytesperline=%u sizeimage=%u\n",
               fmt.fmt.pix.width, fmt.fmt.pix.height, fourcc,
               fmt.fmt.pix.bytesperline, fmt.fmt.pix.sizeimage);

        if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_YUYV) {
                fprintf(stderr, "Driver did not grant YUYV; refusing to save mislabeled raw data\n");
                goto cleanup;
        }

        struct v4l2_requestbuffers req;
        memset(&req, 0, sizeof(req));
        req.count = BUFFER_COUNT;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;

        if (xioctl(fd, VIDIOC_REQBUFS, &req) == -1) {
                perror("VIDIOC_REQBUFS");
                goto cleanup;
        }
        if (req.count < 2) {
                fprintf(stderr, "Driver granted too few buffers: %u\n", req.count);
                goto cleanup;
        }

        unsigned int nbufs = req.count;
        if (nbufs > BUFFER_COUNT)
                nbufs = BUFFER_COUNT;

        for (mapped = 0; mapped < nbufs; mapped++) {
                struct v4l2_buffer buf;

                memset(&buf, 0, sizeof(buf));
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                buf.index = mapped;

                if (xioctl(fd, VIDIOC_QUERYBUF, &buf) == -1) {
                        perror("VIDIOC_QUERYBUF");
                        goto cleanup;
                }

                buffers[mapped].length = buf.length;
                buffers[mapped].start = mmap(NULL, buf.length,
                                             PROT_READ | PROT_WRITE,
                                             MAP_SHARED, fd, buf.m.offset);
                if (buffers[mapped].start == MAP_FAILED) {
                        buffers[mapped].start = NULL;
                        perror("mmap");
                        goto cleanup;
                }
        }

        for (unsigned int queued = 0; queued < mapped; queued++) {
                if (queue_buffer(fd, queued) == -1)
                        goto cleanup;
        }

        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (xioctl(fd, VIDIOC_STREAMON, &type) == -1) {
                perror("VIDIOC_STREAMON");
                goto cleanup;
        }
        streaming = 1;

        for (unsigned int captured = 0; captured < frame_count; captured++) {
                struct pollfd pfd;
                struct v4l2_buffer buf;

                pfd.fd = fd;
                pfd.events = POLLIN;
                pfd.revents = 0;

                if (poll(&pfd, 1, 5000) == -1) {
                        if (errno == EINTR) {
                                captured--;
                                continue;
                        }
                        perror("poll");
                        goto cleanup;
                }
                if (!pfd.revents) {
                        fprintf(stderr, "Timed out waiting for frame %u\n",
                                captured);
                        goto cleanup;
                }

                memset(&buf, 0, sizeof(buf));
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;

                if (xioctl(fd, VIDIOC_DQBUF, &buf) == -1) {
                        if (errno == EAGAIN) {
                                captured--;
                                continue;
                        }
                        perror("VIDIOC_DQBUF");
                        goto cleanup;
                }

                if (buf.index >= mapped) {
                        fprintf(stderr, "Driver returned invalid buffer index %u\n",
                                buf.index);
                        goto cleanup;
                }

                if (buf.flags & V4L2_BUF_FLAG_ERROR)
                        fprintf(stderr, "Frame %u has V4L2_BUF_FLAG_ERROR\n",
                                captured);

                ssize_t written = write(out_fd, buffers[buf.index].start,
                                        buf.bytesused);
                if (written < 0 || (unsigned int)written != buf.bytesused) {
                        perror("write output");
                        goto cleanup;
                }

                printf("Frame %u: index=%u bytesused=%u sequence=%u\n",
                       captured, buf.index, buf.bytesused, buf.sequence);

                if (queue_buffer(fd, buf.index) == -1)
                        goto cleanup;
        }

        ret = EXIT_SUCCESS;

cleanup:
        if (streaming) {
                enum v4l2_buf_type stop_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

                if (xioctl(fd, VIDIOC_STREAMOFF, &stop_type) == -1)
                        perror("VIDIOC_STREAMOFF");
        }

        for (unsigned int i = 0; i < mapped; i++) {
                if (buffers[i].start)
                        munmap(buffers[i].start, buffers[i].length);
        }

        if (out_fd != -1)
                close(out_fd);
        if (fd != -1)
                close(fd);

        return ret;
}
