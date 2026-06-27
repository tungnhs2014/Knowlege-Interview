# Chapter 9 - Leveraging the
V4L2 API from the
User Space
The main purpose of device drivers is controlling and leveraging the underlying hardware
while exposing functionalities to users. These users may be applications running in user
space or other kernel drivers. While the two previous chapters dealt with V4L2 device
drivers, in this chapter, we will learn how to take advantage of V4L2 device functionalities
exposed by the kernel. We will start by describing and enumerating user space V4L2 APIs,
and then we will learn how to leverage those APIs to grab video data from the sensor,
including mangling the sensor properties.
This chapter will cover the following topics:
• V4L2 user space APIs
• Video device property management from user space
• Buffer management from user space
• V4L2 user space tools
414 Leveraging the V4L2 API from the User Space
## Technical requirements
In order to make the most out of this chapter, you will need the following:
• Advanced computer architecture knowledge and C programming skills
• Linux kernel v4.19.X sources, available at https://git.kernel.org/pub/
scm/linux/kernel/git/stable/linux.git/refs/tags
## Introduction to V4L2 from user space
The main purpose of writing device drivers is to ease the control and usage of the
underlying device by the application. There are two ways for user space to deal with V4L2
devices: either by using all-in-one utilities such as GStreamer and its gst-* tools or by
writing a dedicated application using user space V4L2 APIs. In this chapter, we only deal
with the code, thus we will cover how to write applications that use the V4L2 API.
## The V4L2 user space API
The V4L2 user space API has a reduced number of functions and a lot of data structures,
all defined in include/uapi/linux/videodev2.h. In this section, we will try to
describe the most important of them—or, better said, the most commonly used. Your
code should include the following header:
```c
#include <linux/videodev2.h>
```
This API relies on the following functions:
• open(): To open a video device
• close(): To close a video device
• ioctl(): To send ioctl commands to the display driver
• mmap(): To memory map a driver-allocated buffer to user space
• read() or write(), depending on the streaming method
This reduced set of APIs is extended by a very large number of ioctl commands, the most
important of which are as follows:
• VIDIOC_QUERYCAP: This is used to query the capabilities of the driver. People
used to say it is used to query the device's capabilities, but this is not true as the
device may be capable of things that are not implemented in the driver. User space
passes a struct v4l2_capability structure, which will be filled by the video
driver with the relevant information.
Introduction to V4L2 from user space 415
• VIDIOC_ENUM_FMT: This is used to enumerate the image formats that are
supported by the driver. The driver user space passes a struct v4l2_fmtdesc
structure, which will be filled by the driver with the relevant information.
• VIDIOC_G_FMT: For a capture device, this is used to get the current image format.
However, for a display device, you use this to get the current display window. In
either case, user space passes a struct v4l2_format structure, which will be
filled by the driver with the relevant information.
• VIDIOC_TRY_FMT should be used when you are unsure about the format to be
submitted to the device. This is used to validate a new image format for a capture
device or a new display window depending on an output (display) device. User
space passes a struct v4l2_format structure with the properties it would like
to apply, and the driver may change the given values if they are not supported. The
application should then check what is granted.
• VIDIOC_S_FMT is used to set a new image format for a capture device or a new
display window for a display (output device). The driver may change the values
passed by user space if they are not supported. The application should check what is
granted if VIDIOC_TRY_FMT is not used first.
• VIDIOC_CROPCAP is used to get the default cropping rectangle based on the
current image size and the current display panel size. The driver fills a struct
v4l2_cropcap structure.
• VIDIOC_G_CROP is used to get the current cropping rectangle. The driver fills
a struct v4l2_crop structure.
• VIDIOC_S_CROP is used to set a new cropping rectangle. The driver fills a struct
v4l2_crop structure. The application should check what is granted.
• VIDIOC_REQBUFS: This ioctl is used to request a number of buffers that can
later be memory mapped. The driver fills a struct v4l2_requestbuffers
structure. As the driver may allocate more or less than the actual number of buffers
requested, the application should check how many buffers are really granted. No
buffer is queued yet after this.
• The VIDIOC_QUERYBUF ioctl is used to get a buffer's information, which can
be used by the mmap() system call in order to map that buffer to user space. The
driver fills a struct v4l2_buffer structure.
416 Leveraging the V4L2 API from the User Space
• VIDIOC_QBUF is used to queue a buffer by passing a struct v4l2_buffer
structure associated with that buffer. On the execution path of this ioctl, the driver
will add this buffer to its list of buffers so that it is filled when there are no more
pending queued buffers before it. Once the buffer is filled, it is passed to the V4L2
core, which maintains its own list (that is, a ready buffer list) and it is moved off the
driver's list of DMA buffers.
• VIDIOC_DQBUF is used to dequeue a filled buffer (from the V4L2's list of ready
buffers for the input device) or a displayed (output device) buffer by passing a
```c
struct v4l2_buffer structure associated with that buffer. This will block
```
if no buffer is ready unless O_NONBLOCK was used with open(), in which case
VIDIOC_DQBUF will immediately return with an EAGAIN error code. You should
call VIDIOC_DQBUF only after STREAMON has been called. In the meantime,
calling this ioctl after STREAMOFF would return -EINVAL.
• VIDIOC_STREAMON is used to turn on streaming. After that, any VIDIOC_QBUF
results in an image are rendered.
• VIDIOC_STREAMOFF is used to turn off streaming. This ioctl removes all buffers.
It actually flushes the buffer queue.
There are many more ioctl commands than those we have just enumerated. There are
actually at least as many ioctls as there are ops in the kernel's v4l2_ioctl_ops data
structure. However, the preceding ioctls are enough to go deeper into the V4L2 user
space API. In this section, we will not go into detail about each data structure. You
should then keep open the include/uapi/linux/videodev2.h file, also available
at https://elixir.bootlin.com/linux/v4.19/source/include/uapi/
linux/videodev2.h, as it contains all the V4L2 APIs and data structures. That being
said, the following pseudo-code shows a typical ioctl sequence to grab video from user
space using V4L2 APIs:
```c
open()
int ioctl(int fd, VIDIOC_QUERYCAP,
struct v4l2_capability *argp)
int ioctl(int fd, VIDIOC_S_FMT, struct v4l2_format *argp)
int ioctl(int fd, VIDIOC_S_FMT, struct v4l2_format *argp)
```
/* requesting N buffers */
```c
int ioctl(int fd, VIDIOC_REQBUFS,
struct v4l2_requestbuffers *argp)
```
/* queueing N buffers */
```c
int ioctl(int fd, VIDIOC_QBUF, struct v4l2_buffer *argp)
```
/* start streaming */
Introduction to V4L2 from user space 417
```c
int ioctl(int fd, VIDIOC_STREAMON, const int *argp)
```
read_loop: (for i=0; I < N; i++)
/* Dequeue buffer i */
```c
int ioctl(int fd, VIDIOC_DQBUF, struct v4l2_buffer *argp)
process_buffer(i)
```
/* Requeue buffer i */
```c
int ioctl(int fd, VIDIOC_QBUF, struct v4l2_buffer *argp)
```
end_loop
```c
releases_memories()
close()
```
The preceding sequence will serve as a guideline to deal with the V4L2 API in user space.
Be aware that it is possible for the ioctl system call to return a -1 value while errno
= EINTR. In this case, it would not mean an error but simply that the system call was
interrupted, in which case it should be tried again. To address this (rare but possible)
issue, we can consider writing our own wrapper for ioctl, such as the following:
```c
static int xioctl(int fh, int request, void *arg)
{
int r;
```
do {
r = ioctl(fh, request, arg);
```c
} while (-1 == r && EINTR == errno);
return r;
}
```
Now that we are done with the video grabbing sequence overview, we can figure out what
steps are required to proceed to video streaming from device opening to closing, through
format negotiation. We can now jump to the code, starting with the device opening, from
which everything begins.
418 Leveraging the V4L2 API from the User Space
Video device opening and property
management
Drivers expose node entries in the /dev/ directory corresponding to the video interfaces
they are responsible for. These file nodes correspond to the /dev/videoX special files
for capture devices (in our case). The application must open the appropriate file node
prior to any interaction with the video device. It uses the open() system call for that,
which will return a file descriptor that will be the entry point for any command sent to
the device, as in the following example:
```c
static const char *dev_name = "/dev/video0";
```
fd = open (dev_name, O_RDWR);
```c
if (fd == -1) {
```
perror("Failed to open capture device\n");
```c
return -1;
}
```
The preceding snippet is an opening in blocking mode. Passing O_NONBLOCK to open()
would prevent the application from being blocked if there is no ready buffer while
trying to dequeue. Once you're done with the video device, it should be closed using the
```c
close() system call:
```
close (fd);
After we are able to open the video device, we can start our interaction with it. Generally,
the first action that takes place once the video device is opened is to query its capabilities,
through which we can make it operate optimally.
## Querying the device capabilities
It is common to query the capabilities of the device in order to make sure it supports the
mode we need to work with. You do this using the VIDIOC_QUERYCAP ioctl command.
To achieve this, the application passes a struct v4l2_capability structure (defined
in include/uapi/linux/videodev2.h), which will be filled by the driver. This
structure has a .capabilities field that has to be checked. That field contains the
capabilities of the whole device. The following excerpt from the kernel source shows the
possible values:
/* Values for 'capabilities' field */
```c
#define V4L2_CAP_VIDEO_CAPTURE 0x00000001 /*video capture
```
device*/
Video device opening and property management 419
```c
#define V4L2_CAP_VIDEO_OUTPUT 0x00000002 /*video output
```
device*/
```c
#define V4L2_CAP_VIDEO_OVERLAY 0x00000004 /*Can do video
```
overlay*/
[...] /* VBI device skipped */
/* video capture device that supports multiplanar formats */
```c
#define V4L2_CAP_VIDEO_CAPTURE_MPLANE 0x00001000
```
/* video output device that supports multiplanar formats */
```c
#define V4L2_CAP_VIDEO_OUTPUT_MPLANE 0x00002000
```
/* mem-to-mem device that supports multiplanar formats */
```c
#define V4L2_CAP_VIDEO_M2M_MPLANE 0x00004000
```
/* Is a video mem-to-mem device */
```c
#define V4L2_CAP_VIDEO_M2M 0x00008000
```
[...] /* radio, tunner and sdr devices skipped */
```c
#define V4L2_CAP_READWRITE 0x01000000 /*read/write
```
systemcalls */
```c
#define V4L2_CAP_ASYNCIO 0x02000000 /* async I/O */
#define V4L2_CAP_STREAMING 0x04000000 /* streaming I/O
ioctls */
#define V4L2_CAP_TOUCH 0x10000000 /* Is a touch device */
```
The following code block shows a common use case that shows how to query the device
capabilities from the code using the VIDIOC_QUERYCAP ioctl:
```c
#include <linux/videodev2.h>
```
[...]
```c
struct v4l2_capability cap;
```
memset(&cap, 0, sizeof(cap));
```c
if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
if (EINVAL == errno) {
fprintf(stderr, "%s is no V4L2 device\n", dev_name);
```
exit(EXIT_FAILURE);
```c
} else {
```
errno_exit("VIDIOC_QUERYCAP"
```c
}
}
```
420 Leveraging the V4L2 API from the User Space
In the preceding code, struct v4l2_capability is first zeroed thanks to memset()
prior to being given to the ioctl command. At this step, if no error occurs, then our cap
variable now contains the device capabilities. You can use the following to check for the
device type and the I/O methods:
```c
if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
fprintf(stderr, "%s is not a video capture device\n",
```
dev_name);
exit(EXIT_FAILURE);
```c
}
if (!(cap.capabilities & V4L2_CAP_READWRITE))
fprintf(stderr, "%s does not support read i/o\n",
```
dev_name);
/* Check whether USERPTR and/or MMAP method are supported */
```c
if (!(cap.capabilities & V4L2_CAP_STREAMING))
fprintf(stderr, "%s does not support streaming i/o\n",
```
dev_name);
/* Check whether driver support read/write i/o */
```c
if (!(cap.capabilities & V4L2_CAP_READWRITE))
fprintf (stderr, "%s does not support read i/o\n",
```
dev_name);
You may have noticed that we first zeroed our cap variable prior to using it. It is good
practice to always clear parameters that will be given to V4L2 APIs in order to avoid stale
content. Let's then define a macro—say, CLEAR—that will zero any variable given as
a parameter, and use it in the rest of the chapter:
```c
#define CLEAR(x) memset(&(x), 0, sizeof(x))
```
Now, we are done with querying the video device capabilities. This allows us to configure
the device and tweak the image format according to what we need to achieve. By
negotiating the appropriate image format, we can leverage the video device, as we will see
in the next section.
Buffer management 421
## Buffer management
You should consider that in V4L2, two buffer queues are maintained: one for the driver
(referred to as the INPUT queue) and one for the user (referred to as the OUTPUT
queue). Buffers are queued into the driver's queue by the user space application in order
to be filled with data (the application uses the VIDIOC_QBUF ioctl for this). Buffers
are filled by the driver in the order they have been enqueued. Once filled, each buffer is
moved off the INPUT queue and put into the OUTPUT queue, which is the user queue.
Whenever the user application calls VIDIOC_DQBUF in order to dequeue a buffer, this
buffer is looked for in the OUTPUT queue. If it's in there, the buffer will be dequeued and
pushed to the user application; otherwise, the application will wait until a filled buffer is
there. After the user finishes using the buffer, it must call VIDIOC_QBUF on this buffer in
order to enqueue it back in the INPUT queue so that it can be filled again.
After driver initialization, the application calls the VIDIOC_REQBUFS ioctl to set the
number of buffers it needs to work with. Once this is granted, the application queues
all the buffers using VIDIOC_QBUF, and then calls the VIDIOC_STREAMON ioctl.
Then, the driver goes ahead on its own and fills all the queued buffers. If there are
no more queued buffers, then the driver will be waiting for a buffer to be queued in
by the application. If such a case arises, then it means that some frames are lost in the
capture itself.
## Image (buffer) format
After making sure that the device is of the correct type and supports the modes it can
work with, the application must negotiate the video format it needs. The application
has to make sure that the video device is configured to send video frames in a format
that the application can deal with. It has to do this before starting to grab and gather data
(or video frames). The V4L2 API uses struct v4l2_format to represent the buffer
format, whatever the type of the device is. This structure is defined as follows:
```c
struct v4l2_format {
```
u32 type;
union {
```c
struct v4l2_pix_format pix; /* V4L2_BUF_TYPE_VIDEO_CAPTURE */
struct v4l2_pix_format_mplane pix_mp; /* _CAPTURE_MPLANE */
struct v4l2_window win; /* V4L2_BUF_TYPE_VIDEO_OVERLAY */
struct v4l2_vbi_format vbi; /* V4L2_BUF_TYPE_VBI_CAPTURE */
struct v4l2_sliced_vbi_format sliced;/*_SLICED_VBI_CAPTURE */
struct v4l2_sdr_format sdr; /* V4L2_BUF_TYPE_SDR_CAPTURE */
```
422 Leveraging the V4L2 API from the User Space
```c
struct v4l2_meta_format meta;/* V4L2_BUF_TYPE_META_CAPTURE */
```
[...]
```c
} fmt;
};
```
In the preceding structure, the type field represents the type of the data stream and
should be set by the application. Depending on its value, the fmt field will be of the
appropriate type. In our case, type must be V4L2_BUF_TYPE_VIDEO_CAPTURE as
we are dealing with video capture devices. fmt will then be of the struct v4l2_pix_
format type.
Important note
Almost all (if not all) ioctls playing directly or indirectly with the buffer (such
as cropping, buffer requesting/queue/dequeue/querying) need to specify the
buffer type, which makes sense. We will use V4L2_BUF_TYPE_VIDEO_
CAPTURE as it is the only choice we have for our device type. The whole list of
buffer types is of the enum v4l2_buf_type type defined in include/
uapi/linux/videodev2.h. You should have a look.
It is common for applications to query the current format of the video device and then
only change the properties of interest in it, and send back the new, mangled buffer
format to the video device. However, this is not mandatory. We have only done it here
to demonstrate how you can either get or set the current format. The application queries
the current buffer format using the VIDIOC_G_FMT ioctl command. It has to pass a fresh
(by fresh, I mean zeroed) struct v4l2_format structure with the type field set.
The driver will fill the rest in the return path of the ioctl. The following is an example:
```c
struct v4l2_format fmt;
CLEAR(fmt);
```
/* Get the current format */
fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
```c
if (ioctl(fd, VIDIOC_G_FMT, &fmt)) {
printf("Getting format failed\n");
```
exit(2);
```c
}
```
Buffer management 423
Once we have the current format, we can change the relevant properties and send back the
new format to the device. These properties may be pixel format, memory organization for
each color component, and interlaced capture memory organization for each field. We can
also describe the size and pitch of the buffer. Common (but not the only) pixel formats
supported by devices are as follows:
• V4L2_PIX_FMT_YUYV: YUV422 (interleaved)
• V4L2_PIX_FMT_NV12: YUV420 (semi-planar)
• V4L2_PIX_FMT_NV16: YUV422 (semi-planar)
• V4L2_PIX_FMT_RGB24: RGB888 (packed)
Now, let's write the piece of code that changes the properties we need. However, sending
the new format to the video device requires using a new ioctl command—that is,
VIDIOC_S_FMT:
```c
#define WIDTH 1920
#define HEIGHT 1080
#define PIXFMT V4L2_PIX_FMT_YUV420
```
/* Changing required properties and set the format */
fmt.fmt.pix.width = WIDTH;
fmt.fmt.pix.height = HEIGHT;
fmt.fmt.pix.bytesperline = fmt.fmt.pix.width * 2u;
fmt.fmt.pix.sizeimage = fmt.fmt.pix.bytesperline *
fmt.fmt.pix.height;
fmt.fmt.pix.colorspace = V4L2_COLORSPACE_REC709;
fmt.fmt.pix.field = V4L2_FIELD_ANY;
fmt.fmt.pix.pixelformat = PIXFMT;
fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
```c
if (xioctl(fd, VIDIOC_S_FMT, &fmt)) {
printf("Setting format failed\n");
```
exit(2);
```c
}
```
424 Leveraging the V4L2 API from the User Space
Important note
We could have used the preceding code without needing the current format.
The ioctl may succeed. However, this does not mean your parameters have been applied
as is. By default, a device may not support every combination of image width and height,
or even the required pixel format. In this case, the driver will apply the closest values
it supports according to the ones you requested. You then have to check whether your
parameters have been accepted or whether the ones that are granted are good enough
for you to proceed:
```c
if (fmt.fmt.pix.pixelformat != PIXFMT)
printf("Driver didn't accept our format. Can't proceed.\n");
```
/* because VIDIOC_S_FMT may change width and height */
```c
if ((fmt.fmt.pix.width != WIDTH) ||
```
(fmt.fmt.pix.height != HEIGHT))
```c
fprintf(stderr, "Warning: driver is sending image at %dx%d\n",
```
fmt.fmt.pix.width, fmt.fmt.pix.height);
We can even go further by changing the streaming parameters, such as the number of
frames per second. We can achieve this by doing the following:
• Using the VIDIOC_G_PARM ioctl to query the video device's streaming parameters.
This ioctl accepts as a parameter a fresh struct v4l2_streamparm structure
with its type member set. This type should be one of the enum v4l2_buf_type
values.
• Checking v4l2_streamparm.parm.capture.capability and making
sure the V4L2_CAP_TIMEPERFRAME flag is set. This means that the driver allows
changing the capture frame rate.
If so, we can (optionally) use the VIDIOC_ENUM_FRAMEINTERVALS ioctl in order
to get the list of possible frame intervals (the API uses the frame interval, which is
the inverse of the frame rate).
• Using the VIDIOC_S_PARM ioctl and filling in the v4l2_streamparm.parm.
capture.timeperframe members with the appropriate values. That should
allow setting the capture-side frame rate. It's your task to make sure you're reading
fast enough to not get frame drops.
Buffer management 425
The following is an example:
```c
#define FRAMERATE 30
struct v4l2_streamparm parm;
int error;
CLEAR(parm);
```
parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
/* first query streaming parameters */
error = xioctl(fd, VIDIOC_G_PARM, &parm);
```c
if (!error) {
```
/* Now determine if the FPS selection is supported */
```c
if (parm.parm.capture.capability & V4L2_CAP_TIMEPERFRAME) {
```
/* yes we can */
```c
CLEAR(parm);
```
parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
parm.parm.capture.capturemode = 0;
parm.parm.capture.timeperframe.numerator = 1;
parm.parm.capture.timeperframe.denominator = FRAMERATE;
error = xioctl(fd, VIDIOC_S_PARM, &parm);
```c
if (error)
printf("Unable to set the FPS\n");
```
else
/* once again, driver may have changed our requested
* framerate */
```c
if (FRAMERATE !=
```
parm.parm.capture.timeperframe.denominator)
```c
printf ("fps coerced .....: from %d to %d\n",
```
FRAMERATE,
parm.parm.capture.timeperframe.denominator);
Now, we can negotiate image formats and set the streaming parameter. The next logical
continuation would be requesting buffers and proceeding to further processing.
426 Leveraging the V4L2 API from the User Space
## Requesting buffers
Once done with format preparation, it is time to instruct the driver to allocate memory
that is to be used to store video frames. The VIDIOC_REQBUFS ioctl is there to achieve
this. This ioctl takes a fresh struct v4l2_requestbuffers structure as an
argument. Prior to being given to the ioctl, v4l2_requestbuffers must have some
of its fields set:
• v4l2_requestbuffers.count: This member should be set with the number
of memory buffers to be allocated. This member should be set with a value ensuring
that frames won't be dropped because of a lack of queued buffers in the INPUT
queue. Most of the time, 3 or 4 are correct values. Therefore, the driver may not
be comfortable with the requested number of buffers. In this case, the driver will
set v4l2_requestbuffers.count with the granted number of buffers on the
```c
return path of the ioctl. The application should then check this value in order to
```
make sure this granted value fits its needs.
• v4l2_requestbuffers.type: This must be set with the video buffer type, of
the enum 4l2_buf_type type. Here, again, we use V4L2_BUF_TYPE_VIDEO_
CAPTURE. This would be V4L2_BUF_TYPE_VIDEO_OUTPUT for an output
device, for example.
• v4l2_requestbuffers.memory: This must be one of the possible enum
v4l2_memory values. Possible values of interest are V4L2_MEMORY_MMAP,
V4L2_MEMORY_USERPTR, and V4L2_MEMORY_DMABUF. These are all streaming
methods. However, depending on the value of this member, the application may
have additional tasks to perform.
Unfortunately, the VIDIOC_REQBUFS command is the only way for an application
to discover which types of streaming I/O buffer are supported by a given driver. The
application can then try VIDIOC_REQBUFS with each of these values and adapt its
logic according to which one failed or succeeded.
Buffer management 427
## Requesting user pointer buffers – VIDIOC_REQBUFS and malloc
This step involves the driver supporting streaming mode, especially user pointer I/O
mode. Here, the application informs the driver that it is about to allocate a given number
of buffers:
```c
#define BUF_COUNT 4
struct v4l2_requestbuffers req;
```
CLEAR (req);
req.count = BUF_COUNT;
req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
req.memory = V4L2_MEMORY_USERPTR;
```c
if (-1 == xioctl (fd, VIDIOC_REQBUFS, &req)) {
if (EINVAL == errno)
fprintf(stderr,
```
"%s does not support user pointer i/o\n",
dev_name);
else
```c
fprintf("VIDIOC_REQBUFS failed \n");
}
```
Then, the application allocates the buffer memory from user space:
```c
struct buffer_addr {
void *start;
```
size_t length;
```c
};
struct buffer_addr *buf_addr;
int i;
```
buf_addr = calloc(BUF_COUNT, sizeof (*buffer_addr));
```c
if (!buf_addr) {
fprintf(stderr, "Out of memory\n");
```
exit (EXIT_FAILURE);
```c
}
```
428 Leveraging the V4L2 API from the User Space
```c
for (i = 0; i < BUF_COUNT; ++i) {
```
buf_addr[i].length = buffer_size;
buf_addr[i].start = malloc(buffer_size);
```c
if (!buf_addr[i].start) {
fprintf(stderr, "Out of memory\n");
```
exit(EXIT_FAILURE);
```c
}
}
```
This is the first type of streaming, where buffers are malloced in user space and given
to the kernel in order to be filled with video data: the so-called user pointer I/O mode.
There is another fancy streaming mode, where almost everything is done from the kernel.
Without delay, let's introduce it.
Requesting the memory mappable buffer – VIDIOC_REQBUFS,
VIDIOC_QUERYBUF, and mmap
In driver buffer mode, this ioctl also returns the actual number of buffers allocated in the
count member of the v4l2_requestbuffer structure. This streaming method also
requires a new data structure, struct v4l2_buffer. After buffers are allocated by
the driver in the kernel, this structure is used along with the VIDIOC_QUERYBUFS ioctl
in order to query the physical address of each allocated buffer, which can be used with
the mmap() system call. The physical address returned from the driver will be stored in
buffer.m.offset.
The following code excerpt instructs the driver to allocate memory buffers and check the
number of buffers granted:
```c
#define BUF_COUNT_MIN 3
struct v4l2_requestbuffers req; CLEAR (req);
```
req.count = BUF_COUNT;
req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
req.memory = V4L2_MEMORY_MMAP;
```c
if (-1 == xioctl (fd, VIDIOC_REQBUFS, &req)) {
if (EINVAL == errno)
```
Buffer management 429
```c
fprintf(stderr, "%s does not support memory mapping\n",
```
dev_name);
else
```c
fprintf("VIDIOC_REQBUFS failed \n");
}
```
/* driver may have granted less than the number of buffers we
* requested let's then make sure it is not less than the
* minimum we can deal with
*/
```c
if (req.count < BUF_COUNT_MIN) {
fprintf(stderr, "Insufficient buffer memory on %s\n",
```
dev_name);
exit (EXIT_FAILURE);
```c
}
```
After this, the application should call the VIDIOC_QUERYBUF ioctl on each allocated
buffer in order to get their corresponding physical addresses, as the following example
shows:
```c
struct buffer_addr {
void *start;
```
size_t length;
```c
};
struct buffer_addr *buf_addr;
```
buf_addr = calloc(BUF_COUNT, sizeof (*buffer_addr));
```c
if (!buf_addr) {
fprintf (stderr, "Out of memory\n");
```
exit (EXIT_FAILURE);
```c
}
for (i = 0; i < req.count; ++i) {
struct v4l2_buffer buf;
```
CLEAR (buf);
430 Leveraging the V4L2 API from the User Space
buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
buf.memory = V4L2_MEMORY_MMAP; buf.index = i;
```c
if (-1 == xioctl (fd, VIDIOC_QUERYBUF, &buf))
```
errno_exit("VIDIOC_QUERYBUF");
buf_addr[i].length = buf.length;
buf_addr[i].start =
```c
mmap (NULL /* start anywhere */, buf.length,
```
PROT_READ | PROT_WRITE /* required */,
MAP_SHARED /* recommended */, fd, buf.m.offset);
```c
if (MAP_FAILED == buf_addr[i].start)
```
errno_exit("mmap");
```c
}
```
In order for the application to internally track the memory mapping (obtained with
```c
mmap()) of each buffer, we defined a custom data structure, struct buffer_addr,
```
allocated for each granted buffer, which will hold the mapping corresponding to this
buffer.
Requesting DMABUF buffers – VIDIOC_REQBUFS, VIDIOC_EXPBUF,
and mmap
DMABUF is mostly used on mem2mem devices and introduces the concept of the exporter
and importer. Say driver A wants to use buffers created by driver B; then we call B as the
exporter and A as the buffer user/importer.
The export method instructs the driver to export its DMA buffers to user space by
means of the file descriptor. The application achieves this using the VIDIOC_EXPBUF
```c
ioctl and requires a new data structure, struct v4l2_exportbuffer. On the return
```
path of this ioctl, the driver will set the v4l2_requestbuffers.md member with the
file descriptor corresponding to the given buffer. This is a DMABUF file descriptor:
/* V4L2 DMABuf export */
```c
struct v4l2_requestbuffers req;
```
CLEAR (req);
req.count = BUF_COUNT;
req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
Buffer management 431
req.memory = V4L2_MEMORY_DMABUF;
```c
if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req))
```
errno_exit ("VIDIOC_QUERYBUFS");
It is possible for the application to export those buffers as DMABUF file descriptors so
that they can be memory mapped to access the captured video content. The application
should use the VIDIOC_EXPBUF ioctl for this. This ioctl extends the memory mapping
I/O method, so it is only available for V4L2_MEMORY_MMAP buffers. However, it is
actually useless at exporting capture buffers using VIDIOC_EXPBUF and then mapping
them. You should use V4L2_MEMORY_MMAP instead.
VIDIOC_EXPBUF becomes very interesting when it comes to V4L2 output devices.
This way, the application allocates buffers on both capture and output devices using the
VIDIOC_REQBUFS ioctl, and then the application exports the output device's buffers as
DMABUF file descriptors and uses these file descriptors to set the v4l2_buffer.m.fd
field before the enqueueing ioctl on the capture device. The queued buffer will then have
its counterpart (the output device buffer corresponding to v4l2_buffer.m.fd) filled.
In the following example, we export output device buffers as DMABUF file descriptors.
This assumes buffers for this output device have been allocated using the VIDIOC_
REQBUFS ioctl with req.type set to V4L2_BUF_TYPE_VIDEO_OUTPUT and req.
memory set to V4L2_MEMORY_DMABUF:
```c
int outdev_dmabuf_fd[BUF_COUNT] = {-1};
int i;
for (i = 0; i < req.count; i++) {
struct v4l2_exportbuffer expbuf;
```
CLEAR (expbuf);
expbuf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
expbuf.index = i;
```c
if (-1 == xioctl(fd, VIDIOC_EXPBUF, &expbuf)
```
errno_exit ("VIDIOC_EXPBUF");
outdev_dmabuf_fd[i] = expbuf.fd;
```c
}
```
432 Leveraging the V4L2 API from the User Space
Now, we have learned about DMABUF-based streaming and introduced the concepts it
comes with. The next and last streaming method is much simpler and requires less code.
Let's jump to it.
## Requesting read/write I/O memory
This is the simpler streaming mode from a coding point of view. In the case of read/write
I/O, there is nothing to do except to allocate the memory location where the application
will store the read data, as in the following example:
```c
struct buffer_addr {
void *start;
```
size_t length;
```c
};
struct buffer_addr *buf_addr;
```
buf_addr = calloc(1, sizeof(*buf_addr));
```c
if (!buf_addr) {
fprintf(stderr, "Out of memory\n");
```
exit(EXIT_FAILURE);
```c
}
```
buf_addr[0].length = buffer_size;
buf_addr[0].start = malloc(buffer_size);
```c
if (!buf_addr[0].start) {
fprintf(stderr, "Out of memory\n");
```
exit(EXIT_FAILURE);
```c
}
```
In the previous code snippet, we have defined the same custom data structure, struct
buffer_addr. However, there is no real buffer request here (VIDIOC_REQBUFS is not
used) because nothing goes to the kernel yet. The buffer memory is simply allocated and
that is all.
Now, we are done with buffer requests. The next step is to enqueue the requested buffers
so that they can be filled with video data by the kernel. Let's now see how to do this.
Buffer management 433
## Enqueueing the buffer and enabling streaming
Prior to a buffer being accessed and its data being read, this buffer must be enqueued. This
consists of using the VIDIOC_QBUF ioctl on the buffer when using the streaming I/O
method (everything except read/write I/O). Enqueueing a buffer will lock the memory
pages of that buffer in physical memory. This way, those pages cannot be swapped out
to disk. Do note that those buffers remain locked until they are dequeued, until the
VIDIOC_STREAMOFF or VIDIOC_REQBUFS ioctls are called, or until the device is
closed.
In the V4L2 context, locking a buffer means passing this buffer to the driver for hardware
access (usually DMA). If an application accesses (reads/writes) a locked buffer, then the
result is undefined.
To enqueue a buffer, the application must prepare struct v4l2_buffer, and v4l2_
buffer.type, v4l2_buffer.memory, and v4l2_buffer.index should be set
according to the buffer type, the streaming mode, and the index of the buffer when it has
been allocated. Other fields depend on the streaming mode.
Important note
The read/write I/O method does not require enqueueing.
## The concept of prime buffers
For capturing applications, it is customary to enqueue a number (most of the time, the
number of allocated buffers) of empty buffers before you start capturing and enter the
read loop. This helps improve the smoothness of the application and prevent it from being
blocked because of the lack of a filled buffer. This should be done right after the buffers are
allocated.
## Enqueuing user pointer buffers
To enqueue a user pointer buffer, the application must set the v4l2_buffer.
memory member to V4L2_MEMORY_USERPTR. The particularity here is the v4l2_
buffer.m.userptr field, which must be set with the address of the buffer previously
allocated and v4l2_buffer.length set to its size. When the multi-planar API is used,
the m.userptr and length members of the passed array of struct v4l2_plane
have to be used instead:
/* Prime buffers */
```c
for (i = 0; i < BUF_COUNT; ++i) {
struct v4l2_buffer buf;
```
434 Leveraging the V4L2 API from the User Space
```c
CLEAR(buf);
```
buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
buf.memory = V4L2_MEMORY_USERPTR; buf.index = i;
buf.m.userptr = (unsigned long)buf_addr[i].start;
buf.length = buf_addr[i].length;
```c
if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
```
errno_exit("VIDIOC_QBUF");
```c
}
```
## Enqueuing memory mappable buffers
To enqueue a memory mappable buffer, the application must fill struct v4l2_buffer
by setting the type, memory (which must be V4L2_MEMORY_MMAP), and index
members, just as in the following excerpt:
/* Prime buffers */
```c
for (i = 0; i < BUF_COUNT; ++i) {
struct v4l2_buffer buf; CLEAR (buf);
```
buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
buf.memory = V4L2_MEMORY_MMAP;
buf.index = i;
```c
if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
```
errno_exit ("VIDIOC_QBUF");
```c
}
```
## Enqueuing DMABUF buffers
To enqueue an output device's DMABUF buffer into the capture device's one, applications
should fill struct v4l2_buffer, setting the memory field to V4L2_MEMORY_
DMABUF, the type field to V4L2_BUF_TYPE_VIDEO_CAPTURE, and the m.fd field
to a file descriptor associated with a DMABUF buffer of the output device, as in the
following excerpt:
/* Prime buffers */
```c
for (i = 0; i < BUF_COUNT; ++i) {
struct v4l2_buffer buf; CLEAR (buf);
```
Buffer management 435
buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
buf.memory = V4L2_MEMORY_DMABUF; buf.index = i;
buf.m.fd = outdev_dmabuf_fd[i];
/* enqueue the dmabuf to capture device */
```c
if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
```
errno_exit ("VIDIOC_QBUF");
```c
}
```
The preceding code excerpt shows how a V4L2 DMABUF import works. The fd
argument in the ioctl is the file descriptor associated with the capture device, obtained at
the open() syscall. outdev_dmabuf_fd is the array that contains the output device's
DMABUF file descriptors. You may wonder how this can work on output devices that are
not V4L2 but are DRM-compatible, for example. The following is a brief explanation.
First, the DRM subsystem provides APIs in a driver-dependent way, which you
can use to allocate a (dumb) buffer on the GPU, which will return a GEM handle.
The DRM also provides the DRM_IOCTL_PRIME_HANDLE_TO_FD ioctl, which
allows exporting a buffer into the DMABUF file descriptor through PRIME, then the
drmModeAddFB2() API to create a framebuffer object (which is something
that will be read and displayed onscreen, or should I say, the CRT controller, to
be exact) corresponding to this buffer so that it can finally be rendered using the
drmModeSetPlane() or drmModeSetPlane() APIs. The application can then
set the v4l2_requestbuffers.m.fd field with the file descriptor returned by the
DRM_IOCTL_PRIME_HANDLE_TO_FD ioctl. Then, in the read loop, after each VIDIOC_
DQBUF ioctl, the application can change the plane's frame buffer and position using the
drmModeSetPlane() API.
Important note
PRIME is the name of the drm dma-buf interface layer integrated with
GEM, which is one of the memory managers supported by the DRM subsystem
## Enabling streaming
Enabling streaming is kind of like informing V4L2 that the OUTPUT queue will be
accessed as of now. The application should use VIDIOC_STREAMON to achieve this. The
following is an example:
/* Start streaming */
```c
int ret;
int a = V4L2_BUF_TYPE_VIDEO_CAPTURE;
```
436 Leveraging the V4L2 API from the User Space
ret = xioctl(capt.fd, VIDIOC_STREAMON, &a);
```c
if (ret < 0) {
```
perror("VIDIOC_STREAMON\n");
```c
return -1;
}
```
The preceding excerpt is short but is mandatory to enable streaming, without which
buffers can't be dequeued later.
## Dequeuing buffers
This is actually part of the application's read loop. The application dequeues buffers using
the VIDIOC_DQBUF ioctl. This is only possible if the streaming has been enabled before.
When the application calls the VIDIOC_DQBUF ioctl, it instructs the driver to check
whether there are any filled buffers in the OUTPUT queue, and if there are, it outputs
one filled buffer and the ioctl immediately returns. However, if there is no buffer in the
OUTPUT queue, then the application will block (unless the O_NONBLOCK flag has been
set during the open() system call) until a buffer is queued and filled.
Important note
Trying to dequeue a buffer without queuing it first is an error, and the
VIDIOC_DQBUF ioctl should return -EINVAL. When the O_NONBLOCK
flag is given to the open() function, VIDIOC_DQBUF returns immediately
with an EAGAIN error code when no buffer is available.
After dequeuing a buffer and processing its data, the application must immediately queue
back this buffer again so that it can be refilled for the next reading, and so on.
## Dequeuing memory-mapped buffers
The following is an example of dequeuing a buffer that has been memory mapped:
```c
struct v4l2_buffer buf;
```
CLEAR (buf);
buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
buf.memory = V4L2_MEMORY_MMAP;
```c
if (-1 == xioctl (fd, VIDIOC_DQBUF, &buf)) {
switch (errno) {
```
Buffer management 437
```c
case EAGAIN:
return 0;
case EIO:
```
default:
errno_exit ("VIDIOC_DQBUF");
```c
}
}
```
/* make sure the returned index is coherent with the number
* of buffers allocated
*/
assert (buf.index < BUF_COUNT);
/* We use buf.index to point to the correct entry in our
* buf_addr
*/
process_image(buf_addr[buf.index].start);
/* Queue back this buffer again, after processing is done */
```c
if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
```
errno_exit ("VIDIOC_QBUF");
This could have been done in a loop. For example, let's say you need 200 images. The read
loop could look as follows:
```c
#define MAXLOOPCOUNT 200
```
/* Start the loop of capture */
```c
for (i = 0; i < MAXLOOPCOUNT; i++) {
struct v4l2_buffer buf;
```
CLEAR (buf);
buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
buf.memory = V4L2_MEMORY_MMAP;
```c
if (-1 == xioctl (fd, VIDIOC_DQBUF, &buf)) {
```
[...]
```c
}
```
/* Queue back this buffer again, after processing is done */
438 Leveraging the V4L2 API from the User Space
[...]
```c
}
```
This preceding snippet is just a reimplementation of the buffer dequeuing using a loop,
where the counter represents the number of images needed to grab.
## Dequeuing user pointer buffers
The following is an example of dequeuing a buffer using the user pointer:
```c
struct v4l2_buffer buf; int i;
```
CLEAR (buf);
buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
buf.memory = V4L2_MEMORY_USERPTR;
/* Dequeue a captured buffer */
```c
if (-1 == xioctl (fd, VIDIOC_DQBUF, &buf)) {
switch (errno) {
case EAGAIN:
return 0;
case EIO:
```
[...]
default:
errno_exit ("VIDIOC_DQBUF");
```c
}
}
```
/*
* We may need the index to which corresponds this buffer
* in our buf_addr array. This is done by matching address
* returned by the dequeue ioctl with the one stored in our
* array
*/
```c
for (i = 0; i < BUF_COUNT; ++i)
if (buf.m.userptr == (unsigned long)buf_addr[i].start &&
```
buf.length == buf_addr[i].length)
break;
Buffer management 439
/* the corresponding index is used for sanity checks only */
assert (i < BUF_COUNT);
process_image ((void *)buf.m.userptr);
/* requeue the buffer */
```c
if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
```
errno_exit ("VIDIOC_QBUF");
The preceding code shows how to dequeue a user pointer buffer, and is well commented
enough to not require any further explanation. However, this could be implemented in
a loop if many buffers were needed.
## Read/write I/O
This is the last example, showing how to dequeue a buffer using the read() system call:
```c
if (-1 == read (fd, buffers[0].start, buffers[0].length)) {
switch (errno) {
case EAGAIN:
return 0;
case EIO:
```
[...]
default:
errno_exit ("read");
```c
}
}
```
process_image (buffers[0].start);
None of the previous examples have been discussed in detail because each of them uses
a concept that was already introduced in the The V4L2 user space API section. Now that
we are familiar with writing V4L2 user space code, let's see how not to write any code by
using dedicated tools that can be used for quickly prototyping your camera system.
440 Leveraging the V4L2 API from the User Space
## V4L2 user space tools
So far, we have learned how to write user space code to interact with the driver in the
kernel. For rapid prototyping and testing, we could leverage some community-provided
V4L2 user space tools. By using those tools, we can focus on the system design and
validate the camera system. The most well-known tool is v4l2-ctl, which we will focus
on; it is shipped with the v4l-utils package.
Though it is not discussed in this chapter, there is also the yavta tool (which stands for
Yet Another V4L2 Test Application), which can be used to test, debug, and control the
camera subsystem.
## Using v4l2-ctl
v4l2-utils is a user space application that can be used to query or configure V4L2
devices (including subdevices). This tool can help in setting up and designing fine-grained
V4L2-based systems as it helps tweak and leverage the device's features.
Important note
qv4l2 is the Qt GUI equivalent of v4l2-ctl. v4l2-ctl is ideal for
embedded systems, while qv4l2 is ideal for interactive testing.
## Listing the video devices and their capabilities
First of all, we would need to list all the available video devices using the --listdevices option:
# v4l2-ctl --list-devices
Integrated Camera: Integrated C (usb-0000:00:14.0-8):
/dev/video0
/dev/video1
If several devices are available, we can use the -d option after any v4l2-ctl commands
in order to target a specific device. Do note that if the -d option is not specified, /dev/
video0 is targeted by default.
In order to have information on a specific device, you must use the -D option, as follows:
# v4l2-ctl -d /dev/video0 -D
Driver Info (not using libv4l2):
Driver name : uvcvideo
Card type : Integrated Camera: Integrated C
V4L2 user space tools 441
Bus info : usb-0000:00:14.0-8
Driver version: 5.4.60
Capabilities : 0x84A00001
Video Capture
Metadata Capture
Streaming
Extended Pix Format
Device Capabilities
Device Caps : 0x04200001
Video Capture
Streaming
Extended Pix Format
```c
The preceding command shows the device information (such as the driver and its version)
```
as well as its capabilities. That being said, the --all command gives better verbosity. You
should give it a try.
## Changing the device properties (controlling the device)
Before we look at changing the device properties, we first need to know what controls the
device supports, what their value types (integer, Boolean, string, and so on) are, what their
default values are, and what values are accepted.
In order to get the list of controls supported by the device, we can use v4l2-ctl with
the -L option, as follows:
# v4l2-ctl -L
brightness 0x00980900 (int) : min=0 max=255
step=1 default=128 value=128
contrast 0x00980901 (int) : min=0 max=255
step=1 default=32 value=32
saturation 0x00980902 (int) : min=0 max=100
step=1 default=64 value=64
hue 0x00980903 (int) : min=-180 max=180
step=1 default=0 value=0
white_balance_temperature_auto 0x0098090c (bool) : default=1
value=1
gamma 0x00980910 (int) : min=90 max=150
step=1 default=120 value=120
power_line_frequency 0x00980918 (menu) : min=0 max=2
442 Leveraging the V4L2 API from the User Space
default=1 value=1
0: Disabled
1: 50 Hz
2: 60 Hz
white_balance_temperature 0x0098091a (int) : min=2800
max=6500 step=1 default=4600 value=4600 flags=inactive
sharpness 0x0098091b (int) : min=0 max=7
step=1 default=3 value=3
backlight_compensation 0x0098091c (int) : min=0 max=2
step=1 default=1 value=1
exposure_auto 0x009a0901 (menu) : min=0 max=3
default=3 value=3
1: Manual Mode
3: Aperture Priority Mode
exposure_absolute 0x009a0902 (int) : min=5 max=1250
step=1 default=157 value=157 flags=inactive
exposure_auto_priority 0x009a0903 (bool) : default=0
value=1
jma@labcsmart:~$
In the preceding output, the "value=" field returns the current value of the control, and
the other fields are self-explanatory.
Now that we are aware of the list of controls supported by the device, a control value can
be changed thanks to the --set-ctrl option, as in the following example:
# v4l2-ctl --set-ctrl brightness=192
After that, we can check the current value with the following:
# v4l2-ctl -L
brightness 0x00980900 (int) : min=0 max=255
step=1 default=128 value=192
[...]
Or, we could have used the --get-ctrl command, as follows:
# v4l2-ctl --get-ctrl brightness
brightness: 192
V4L2 user space tools 443
Now it may be time to tweak the device. Before that, let's check the video characteristics
of the device.
## Setting the pixel format, resolution, and frame rate
Before selecting a specific format or resolution, we need to enumerate what is available for
the device. In order to get the supported pixel format, as well as the resolution and frame
rate, the --list-formats-ext option needs to be given to v4l2-ctl, as follows:
# v4l2-ctl --list-formats-ext
```c
ioctl: VIDIOC_ENUM_FMT
```
Index : 0
Type : Video Capture
Pixel Format: 'MJPG' (compressed)
Name : Motion-JPEG
Size: Discrete 1280x720
Interval: Discrete 0.033s (30.000 fps)
Size: Discrete 960x540
Interval: Discrete 0.033s (30.000 fps)
Size: Discrete 848x480
Interval: Discrete 0.033s (30.000 fps)
Size: Discrete 640x480
Interval: Discrete 0.033s (30.000 fps)
Size: Discrete 640x360
Interval: Discrete 0.033s (30.000 fps)
Size: Discrete 424x240
Interval: Discrete 0.033s (30.000 fps)
Size: Discrete 352x288
Interval: Discrete 0.033s (30.000 fps)
Size: Discrete 320x240
Interval: Discrete 0.033s (30.000 fps)
Size: Discrete 320x180
Interval: Discrete 0.033s (30.000 fps)
Index : 1
Type : Video Capture
Pixel Format: 'YUYV'
Name : YUYV 4:2:2
444 Leveraging the V4L2 API from the User Space
Size: Discrete 1280x720
Interval: Discrete 0.100s (10.000 fps)
Size: Discrete 960x540
Interval: Discrete 0.067s (15.000 fps)
Size: Discrete 848x480
Interval: Discrete 0.050s (20.000 fps)
Size: Discrete 640x480
Interval: Discrete 0.033s (30.000 fps)
Size: Discrete 640x360
Interval: Discrete 0.033s (30.000 fps)
Size: Discrete 424x240
Interval: Discrete 0.033s (30.000 fps)
Size: Discrete 352x288
Interval: Discrete 0.033s (30.000 fps)
Size: Discrete 320x240
Interval: Discrete 0.033s (30.000 fps)
Size: Discrete 320x180
Interval: Discrete 0.033s (30.000 fps)
From the previous output, we can see what is supported by the target device, which is the
MJPG (mjpeg) compressed format and the YUYV raw format.
Now, in order to change the camera configuration, first select the frame rate using the
--set-parm option, as follows:
# v4l2-ctl --set-parm=30
Frame rate set to 30.000 fps
#
Then, you can select the required resolution and/or pixel format using the --set-fmtvideo option, as follows:
# v4l2-ctl --set-fmt-video=width=640,height=480,
pixelformat=MJPG
When it comes to the frame rate, you would want to use v4l2-ctl with the
--set-parm option, giving the frame rate numerator only—the denominator is fixed to
1 (only integer frame rate values are allowed)—as follows:
# v4l2-ctl --set-parm=<framerate numerator>
V4L2 user space tools 445
## Capturing frames and streaming
v4l2-ctl supports many more options than you can imagine. In order to see the
possible options, you can print the help message of the appropriate section. Common
help commands related to streaming and video capture are the following:
• --help-streaming: Prints the help message for all options that deal with
streaming
• --help-subdev: Prints the help message for all options that deal with
v4l-subdevX devices
• --help-vidcap: Prints the help message for all options that get/set/list video
capture formats
From those help commands, I've built the following command in order to capture
a QVGA MJPG compressed frame on disk:
# v4l2-ctl --set-fmt-video=width=320,height=240,
pixelformat=MJPG \
--stream-mmap --stream-count=1 --stream-to=grab-320x240.mjpg
I've also managed to capture a raw YUV image with the same resolution with the
following command:
# v4l2-ctl --set-fmt-video=width=320,height=240,
pixelformat=YUYV \
--stream-mmap --stream-count=1 --stream-to=grab-320x240-yuyv.
raw
The raw YUV image cannot be displayed unless you use a decent raw image viewer.
In order to do so, the raw image must be converted using the ffmpeg tool, for example,
as follows:
# ffmpeg -f rawvideo -s 320x240 -pix_fmt yuyv422 \
-i grab-320x240-yuyv.raw grab-320x240.png
446 Leveraging the V4L2 API from the User Space
You can notice a big difference in terms of the size between the raw and the compressed
image, as in the following snippet:
# ls -hl grab-320x240.mjpg
-rw-r--r-- 1 root root 8,0K oct. 21 20:26 grab-320x240.mjpg
# ls -hl grab-320x240-yuyv.raw
-rw-r--r-- 1 root root 150K oct. 21 20:26 grab-320x240-yuyv.
raw
Do note that it is good practice to include the image format in the filename of a raw
capture (such as yuyv in grab-320x240-yuyv.raw) so that you can easily convert
from the right format. This rule is not necessary with compressed image formats because
these formats are image container formats with a header that describes the pixel data that
follows, and can be easily read with the gst-typefind-1.0 tool. JPEG is such a format
and the following is how its header can be read:
# gst-typefind-1.0 grab-320x240.mjpg
grab-320x240.mjpg - image/jpeg, width=(int)320,
height=(int)240, sof-marker=(int)0
# gst-typefind-1.0 grab-320x240-yuyv.raw
grab-320x240-yuyv.raw - FAILED: Could not determine type of
stream.
Now that we are done with tool usages, let's see how to go deeper and learn about V4L2
debugging and from user space.
## Debugging V4L2 in user space
Since our video system setup may not be free of bugs, V4L2 provides a simple but large
backdoor for debugging from user space, in order to track and shoot down trouble
coming from either the VL4L2 framework core or the user space API.
Framework debugging can be enabled as follows:
# echo 0x3 > /sys/module/videobuf2_v4l2/parameters/debug
# echo 0x3 > /sys/module/videobuf2_common/parameters/debug
V4L2 user space tools 447
The preceding commands will instruct V4L2 to add core traces to the kernel log message.
This way, it will easily track where the trouble is coming from, assuming it's coming from
the core. Run the following command:
# dmesg
[831707.512821] videobuf2_common: __setup_offsets: buffer 0,
plane 0 offset 0x00000000
[831707.512915] videobuf2_common: __setup_offsets: buffer 1,
plane 0 offset 0x00097000
[831707.513003] videobuf2_common: __setup_offsets: buffer 2,
plane 0 offset 0x0012e000
[831707.513118] videobuf2_common: __setup_offsets: buffer 3,
plane 0 offset 0x001c5000
[831707.513119] videobuf2_common: __vb2_queue_alloc: allocated
4 buffers, 1 plane(s) each
[831707.513169] videobuf2_common: vb2_mmap: buffer 0, plane 0
successfully mapped
[831707.513176] videobuf2_common: vb2_core_qbuf: qbuf of buffer
0 succeeded
[831707.513205] videobuf2_common: vb2_mmap: buffer 1, plane 0
successfully mapped
[831707.513208] videobuf2_common: vb2_core_qbuf: qbuf of buffer
1 succeeded
[...]
In the previous kernel log messages, we can see the kernel-related V4L2 core functions
call, along with some other details. If for any reason the V4L2 core tracing is not necessary
or not enough for you, you can also enable V4L2 userland API tracing with the following
command:
$ echo 0x3 > /sys/class/video4linux/video0/dev_debug
After running the command, allowing you to capture a raw image, we can see the
following in the kernel log messages:
$ dmesg
[833211.742260] video0: VIDIOC_QUERYCAP: driver=uvcvideo,
card=Integrated Camera: Integrated C, bus=usb-0000:00:14.0-8,
version=0x0005043c, capabilities=0x84a00001,
device_caps=0x04200001
[833211.742275] video0: VIDIOC_QUERY_EXT_CTRL: id=0x980900,
448 Leveraging the V4L2 API from the User Space
type=1, name=Brightness, min/max=0/255, step=1, default=128,
flags=0x00000000, elem_size=4, elems=1, nr_of_dims=0,
dims=0,0,0,0
[...]
[833211.742318] video0: VIDIOC_QUERY_EXT_CTRL: id=0x98090c,
type=2, name=White Balance Temperature, Auto, min/max=0/1,
step=1, default=1, flags=0x00000000, elem_size=4, elems=1,
nr_of_dims=0, dims=0,0,0,0
[833211.742365] video0: VIDIOC_QUERY_EXT_CTRL: id=0x98091c,
type=1, name=Backlight Compensation, min/max=0/2, step=1,
default=1, flags=0x00000000, elem_size=4, elems=1,
nr_of_dims=0, dims=0,0,0,0
[833211.742376] video0: VIDIOC_QUERY_EXT_CTRL: id=0x9a0901,
type=3, name=Exposure, Auto, min/max=0/3, step=1, default=3,
flags=0x00000000, elem_size=4, elems=1, nr_of_dims=0,
dims=0,0,0,0
[...]
[833211.756641] videobuf2_common: vb2_mmap: buffer 1, plane 0
successfully mapped
[833211.756646] videobuf2_common: vb2_core_qbuf: qbuf of buffer
1 succeeded
[833211.756649] video0: VIDIOC_QUERYBUF: 00:00:00.00000000
index=2, type=vid-cap, request_fd=0, flags=0x00012000,
field=any, sequence=0, memory=mmap, bytesused=0, offset/
userptr=0x12e000, length=614989
[833211.756657] timecode=00:00:00 type=0, flags=0x00000000,
frames=0, userbits=0x00000000
[833211.756698] videobuf2_common: vb2_mmap: buffer 2, plane 0
successfully mapped
[833211.756704] videobuf2_common: vb2_core_qbuf: qbuf of buffer
2 succeeded
[833211.756706] video0: VIDIOC_QUERYBUF: 00:00:00.00000000
index=3, type=vid-cap, request_fd=0, flags=0x00012000,
field=any, sequence=0, memory=mmap, bytesused=0, offset/
userptr=0x1c5000, length=614989
[833211.756714] timecode=00:00:00 type=0, flags=0x00000000,
frames=0, userbits=0x00000000
[833211.756751] videobuf2_common: vb2_mmap: buffer 3, plane 0
successfully mapped
[833211.756755] videobuf2_common: vb2_core_qbuf: qbuf of buffer
3 succeeded
V4L2 user space tools 449
[833212.967229] videobuf2_common: vb2_core_streamon: successful
[833212.967234] video0: VIDIOC_STREAMON: type=vid-cap
In the preceding output, we can trace the different V4L2 userland API calls, which
correspond to the different ioctl commands and their parameters.
## V4L2 compliance driver testing
In order for a driver to be V4L2-compliant, it must meet some criteria, which includes
passing the v4l2-compliance tool test, which is used to test V4L devices of all kinds.
v4l2-compliance attempts to test almost all aspects of a V4L2 device and it covers
almost all V4L2 ioctls.
As with other V4L2 tools, a video device can be targeted using the -d or --device=
commands. If a device is not specified, /dev/video0 is targeted. The following is an
output excerpt:
# v4l2-compliance
v4l2-compliance SHA : not available
Driver Info:
Driver name : uvcvideo
Card type : Integrated Camera: Integrated C
Bus info : usb-0000:00:14.0-8
Driver version: 5.4.60
Capabilities : 0x84A00001
Video Capture
Metadata Capture
Streaming
Extended Pix Format
Device Capabilities
Device Caps : 0x04200001
Video Capture
Streaming
Extended Pix Format
Compliance test for device /dev/video0 (not using libv4l2):
Required ioctls:
450 Leveraging the V4L2 API from the User Space
test VIDIOC_QUERYCAP: OK
Allow for multiple opens:
test second video open: OK
test VIDIOC_QUERYCAP: OK
test VIDIOC_G/S_PRIORITY: OK
test for unlimited opens: OK
Debug ioctls:
test VIDIOC_DBG_G/S_REGISTER: OK (Not Supported)
test VIDIOC_LOG_STATUS: OK (Not Supported)
[]
Output ioctls:
test VIDIOC_G/S_MODULATOR: OK (Not Supported)
test VIDIOC_G/S_FREQUENCY: OK (Not Supported)
[...]
Test input 0:
Control ioctls:
fail: v4l2-test-controls.cpp(214): missing control class
for class 00980000
fail: v4l2-test-controls.cpp(251): missing control class
for class 009a0000
test VIDIOC_QUERY_EXT_CTRL/QUERYMENU: FAIL
test VIDIOC_QUERYCTRL: OK
fail: v4l2-test-controls.cpp(437): s_ctrl returned an
```c
error (84)
```
test VIDIOC_G/S_CTRL: FAIL
fail: v4l2-test-controls.cpp(675): s_ext_ctrls returned an
error (
Summary 451
In the preceding logs, we can see that /dev/video0 has been targeted. Additionally,
we notice that Debug ioctls and Output ioctls are not supported by our driver
(these are not failures). Though the output is verbose enough, it is better to use the
--verbose command as well, which makes the output more user friendly and much
more detailed. It then goes without saying that if you want to submit a new V4L2 driver,
that driver must pass the V4L2 compliance tests.
## Summary
In this chapter, we walked through the user space implementation of V4L2. We started
with V4L2 buffer management, from video streaming. We also learned how to deal
with video device property management, all from user space. However, V4L2 is a heavy
framework, not just in terms of code but also in terms of power consumption. So, in the
next chapter, we will address Linux kernel power management in order to keep the system
at the lowest consumption level possible without degrading the system properties.