# How to fix mouse cursor latency

Latency is a complex topic! This file contains [several techniques](#reduce-queued-frames) you can use to
reduce the input latency of your applications, as well as [platform-specific considerations](#platform-specific-considerations). First, here is some general background infromation.

Input latency is the time it takes for pixels to change on screen in response to a user input such as a
mouse movement. Dear ImGui adds no particular extra input latency for most behaviors, e.g. the last
value passed to 'io.AddMousePosEvent()' before NewFrame() will result in windows being moved to the right
spot at the time of EndFrame()/Render().

Unfortuantely, your rendering API and/or window manager and/or GPU driver may be adding quite a bit of
input latency to all rendering by default. It is difficult to directly see input latency, and it may be
difficult or even impossible to measure it in software depending on the platform. Developers are often
unaware of how much input latency their application has.

We typically think of the time between 60 Hz frames as very, very small, so it may seem weird to worry
about intervals of time around one frame or less. But to a computer a millisecond is a very long time, and
there are typically 16 milliseconds between frames, during which time the computer can perform billions or
even trillions of operations.

"Sure, computers are fast", you may think, "but humans are slow. Do a few millseconds of latency matter
to humans?" The answer is definitively yes.
Multiple research studies performed using screens with adjustable latency ([1](https://www.youtube.com/watch?v=vOvQCPLkPt4), [2](https://www.semanticscholar.org/paper/In-the-blink-of-an-eye%3A-investigating-latency-Ng-Annett/386a15fd85c162b8e4ebb6023acdce9df2bd43ee?p2df))
have shown that humans can perceive a difference all the way down to 1 millisecond of latency. Which is far
better than can be achieved on any system capable of running Dear ImGui! Some representative values for
end-to-end input latency can be found [here](https://danluu.com/input-lag/).

Input latency can affect the "feel" of an application even when it is too small to see directly. An
application with a lot of input latency may feel generally sluggish despite running at a smooth 60 FPS.
Input latency is most directly noticeable when dragging an object such as a Dear ImGui window with a
mouse or touchscreen.

OS mouse cursors are typically drawn using a hardware overlay and their position is updated at the very
last instant before VSync. This means they have less input latency than almost anything rendered via a
regular graphics API (including, but not limited to, Dear ImGui windows). This _difference_ in input
latency commonly causes a "rubber-banding" effect, where objects appear to lag behind the mouse cursor
(as if attached loosely by a rubber band).

So how can we reduce input latency and the rubber-banding and sluggishness it causes?

# Don't Disable VSync

Disabling VSync is not a good idea. If you're looking for something to implement now, skip to the
["Reduce Queued Frames" section](#reduce-queued-frames). Read on to find out why *not* to disable VSync.

First some terminology. "VSync" or Vertical Sync refers to the signal that the GPU sends once per frame
over the monitor cable, marking the first pixel of a new frame. "Vertical" because the pixels are sent
from top to bottom in rows. (There is also a HSync to mark the beginning of each row.) The pixels to be
shown are stored in GPU memory in what's called the "front buffer". There are many "back buffers" which
applications render into, and then perform a "Present" or "Swap" or "Flip" operation to turn one of the
back buffers into the new front buffer, while the old front buffer becomes a back buffer. The process of
sending pixels from the front buffer through the monitor cable to be displayed is called "scanout". The
period after the end of scanout but before the next VSync is called "VBlank". So VSync marks the end of
VBlank and the beginning of scanout.

In VGA cables there is a literal pin and wire whose only purpose is to send the VSync
signal. In modern digital cables VSync is a data packet sent on the same wires as other data. But it's an
essential part of every GPU to monitor connection and it is always sent. It is never actually disabled.
So what do we mean by "Disable VSync"?

In rendering, VSync often refers to two other concepts besides the monitor signal itself. One is the
concept of making the application wait until the monitor is ready to accept the next frame, throttling
the FPS to the refresh rate of the monitor. Let's call this "backpressure". The other concept is
preventing the application from updating the front buffer during scanout. Let's call this
"tearing prevention".

## Tearing prevention

A tearing artifact occurs when the application updates or swaps the front buffer during scanout. This
results in two frames being shown on different parts of the monitor at the same time, usually with a
visible line where they meet, which jumps around from frame to frame. Some APIs prevent tearing even when
VSync is disabled, and some don't. Some have separate controls for VSync and tearing.

Tearing is a pretty bad artifact, generally worse than a bit of extra input latency. As a result, updating
the front buffer during scanout is usually not desired. However, it can actually be done without tearing
if one is *extremely* careful about timing. This is sometimes called "beam racing" and it has been used in
VR compositors and some home consoles and console emulators. For more information about how this can be
achieved, [see here](https://blurbusters.com/blur-busters-lagless-raster-follower-algorithm-for-emulator-developers/).

It is unlikely that beam racing or tearing can benefit a Dear ImGui app. So for the rest of this document
let's assume that the platform prevents tearing artifacts. How? At the beginning of scanout when VSync is
sent, the platform selects the most recent completed frame buffer as the front buffer, and prevents the
application from modifying it. The application renders the next frame into a different frame buffer (often
using triple buffering). If that frame finishes rendering even one microsecond after VSync, it has to
wait all the way until the next VSync before it can be sent to the monitor. Bummer! But that's the price
of preventing tearing artifacts.

Actually it can be even worse than this. When a compositing window manager is in use, the front buffer is
not one of the application's buffers. The front buffer is the desktop, and it is owned by the window
manager. The window manager draws desktop frames using the same 3D APIs as the application, with double
buffering. At VSync when scanout starts for the desktop front buffer, the window manager wakes up and
starts drawing the next desktop frame. It takes the application's most recently completed frame buffer and
composites it into the desktop back buffer. Then it goes to sleep for the rest of the frame so the
application can do its rendering. But if the application finishes a new frame before VSync, the window
manager doesn't wake up again to re-composite the desktop. At the next VSync the desktop back buffer
swaps and becomes the front buffer, but it still contains the application's old frame. The new application
frame is then composited into the next desktop back buffer and has to wait a whole extra VSync interval
before it is sent to the monitor. So a compositing window manager typically adds a whole extra frame of
input latency to every application.

## Backpressure

Removing backpressure is probably what most people first think of as "disabling VSync". Backpressure is
typically enabled by default. This means that the rendering API will block when the application calls
"Present" or "Swap", making the application wait until the next VSync before it can start rendering the
next frame.

Without backpressure the application will render frames as fast as possible, regardless of the monitor
refresh rate. If the application's FPS is lower than the monitor refresh rate, this has no effect. But
if the application's FPS is much higher than the monitor refresh rate, then removing backpressure may
reduce input latency.

Removing backpressure comes with a lot of downsides! Input latency is only reduced in the case where some
of the frames produced by the application are never sent to the monitor, being replaced by newer frames
before the next VSync. Rendering these garbage frames has no benefit. It wastes power and makes devices
hot and reduces battery life. Not only that, it can even cause the application and the whole device to
run slower overall due to thermal throttling. It would be far better to simply skip rendering the frames
that aren't sent to the monitor and sleep instead. But because the application is ignoring the VSync 
signal, it doesn't know ahead of time which frames will be used.

Even if you don't care about wasting power and don't hit thermal throttling, disabling backpressure
doesn't achieve the minimum theoretical latency. The input latency will be on average 1.5x the time to
render a frame, because on average you'll be halfway done with a frame when VSync happens, and the
previous frame will be sent to the monitor. The latency will fluctuate between 1 and 2 frames at
random. The techniques discussed below can reduce input latency even lower than this, while keeping
backpressure enabled for much higher efficiency.

# Reduce queued frames

The best way to reduce input latency is to reduce the number of queued frames. Most rendering
APIs will queue up multiple frames by default. That is, as you make draw calls and call "Present" or
"Swap", those commands are put on an input queue behind previous frames and not executed on the GPU until
later. Then after the GPU executes the drawing commands, the finished frames may be put on a presentation
queue, waiting to be sent to the monitor. Together these queues may hold many frames.

This buffering can have benefits. For GPU-bound applications the input queue will always be full. The
GPU will never wait for the CPU. It can just pull the drawing commands for the next frame from the input
queue whenever it finishes the previous one. This allows reaching 100% GPU utilization. It also smooths
out variations in frame rate. If one frame takes extra time to render, the GPU can still pull previously
finished frames from the presentation queue to send to the monitor, so the user doesn't see a missed
frame.

But all this queueing has a huge downside. Once frames are in the GPU input queue they generally are
not modified to reflect new mouse or keyboard events that come in later. This increases input latency
by the number of frames that are queued. This is the single biggest source of input latency in many
applications.

If your application can render an entire frame from start to finish in one VSync interval, and doesn't
need 100% GPU utilization, then you can dramatically reduce your input latency by limiting the queue
depth to the minimum possible value. Different rendering APIs have their own ways of limiting queue
length, so you will have to research the methods for your specific API. Some of the examples already
include code to limit the number of queued frames (e.g. SetMaximumFrameLatency for DirectX).

# Process input events as late in your game loop as possible

Any time after processing input events adds to your input latency. If you have tasks to perform that
don't depend on the user input in the current frame, such as allocating buffers or waiting for GPU
resources to become available, then do those first before processing user input. While you are doing
those tasks, more user input events may arrive, which you can then process before rendering, reducing
input latency.

As a very advanced technique, you can process user input twice. Once at the beginnning of your game
loop, and again at the end. Then, if your graphics API allows it, you can update GPU buffers to
modify draw calls that you've already made to reflect the new user input (e.g. updating the position
of a window). This requires that you do extra synchronization between the CPU and GPU to ensure that
the CPU is not modifying data that the GPU is in the process of using. Dear ImGui does not currently
support this technique, as it is probably overkill for most purposes, but it can be useful in other
contexts. For example, VR compositors use this technique to update head tracking data at the last
possible moment before rendering, to reduce nausea in VR.

# Delay rendering until just before VSync

If you have already reduced the number of queued frames to the minimum, it may still be possible to
reduce latency further. The secret to reducing input latency is, counterintuitively, to delay
rendering.

If you start rendering right at VSync and finish early, you will spend the rest of the frame
interval waiting for the next VSync. During that time new mouse and keyboard events may arrive, but your
frame is already rendered and you can't update it until after the next VSync. That waiting time is extra
input latency. Instead, if you move that waiting time to the beginning of the frame, before you render,
then it doesn't contribute to input latency because you can process any mouse and keyboard events that
arrived while you were waiting _before_ you render the frame.

Waiting the perfect amount of time requires measuring and predicting exactly how long your rendering
will take, and knowing exactly when VSync occurs on the monitor your window is displayed on, and
ensuring that the operating system scheduler wakes up your process at exactly the right moment. This
is all surprisingly difficult. If you get it slightly wrong and your frame takes slightly more time
to render than you thought, your frame may not be done in time for VSync. Then it will have to wait
a whole extra frame.

Dear ImGui is compatible with this method, but the examples don't implement it due to its complexity.When implemented well it can reduce input latency to a small fraction of a monitor refresh interval,
beat "disable VSync", and match the hardware rendered mouse cursor almost exactly.

# Variable refresh rate (VRR)

AKA G-Sync, FreeSync, Adaptive-Sync, ProMotion, Q-Sync

Many displays now have the ability to delay VSync on a frame-by-frame basis. With traditional
fixed VSync timing, if rendering a frame takes a bit longer than expected and misses VSync,
it must wait all the way until the next VSync before it can be sent to the monitor. With
variable refresh rate, the GPU delays VSync until the frame is done rendering and _then_ sends the
VSync signal immediately, so finished pixels are sent to the monitor with minimal delay.

This is very beneficial for input latency if your application can't render at the monitor's
maximum refresh rate. If your application can render faster than the monitor's maximum refresh
rate then there is no direct latency benefit. However, VRR works very well together with the previously
described technique of delaying rendering. There is no risk of missing a fixed VSync deadline, so exact
timing is much less critical.

VRR does have some limitations. Many systems don't support it. Many that support it don't enable it
in non-fullscreen applications. Only one application can control VSync at a time; when using VRR in
windowed mode all other applications must render at whatever rate the topmost application chooses
(even if that is a very slow rate such as 24 FPS, or a wildly varying rate). Platform-specific APIs may
be required to enable VRR. Predicting the time rendering will take is still difficult and still important
for applications that want to show smooth animations and/or synchronized audio and video, because the
animation timestep or audio sync depends on when the frame will ultimately be displayed to the user, which
now depends directly on how long rendering takes. Applications may have components such as physics engines
that prefer a fixed timestep, which can make it difficult to render frames at arbitrary intervals.

The Dear ImGui examples do not currently support VRR.

# Software rendered mouse cursor hack

A technique you can use to reduce the _appearance_ of latency is to enable the io.MouseDrawCursor
flag to request Dear ImGui to draw a mouse cursor itself, instead of using the operating system's built in
hardware rendered mouse cursor. This causes the mouse cursor itself to have the same latency as everything
else in your application, so there is no longer a mismatch between the mouse cursor and objects being
dragged.

Unfortunately this does not actually _reduce_ the input latency, so the mouse cursor is likely to
feel sluggish when io.MouseDrawCursor is enabled. Instead of enabling io.MouseDrawCursor all the time,
you may want to enable it _only_ during interactive drag operations.

# Predict user input

Another technique that can reduce the _appearance_ of latency is to predict user input ahead of time. When
you read a mouse position update, then use it to render a frame, you are essentially predicting that the
mouse will remain stationary in the time between the mouse event and the frame becoming visible on the
monitor. But if the mouse is moving, this is a pretty bad prediction. Instead, you might calculate the
mouse velocity, estimate the future time when the rendered frame will appear on the monitor, and use the
velocity to estimate where the mouse *will* be at that time, then use that as the mouse position instead
of the original one. You could use fancier prediction methods such as machine learning. And this technique
can even compensate for latency that occurs in the input device itself, before input events reach your
application, given accurate timestamps on the input events.

This technique is often used by drawing applications with stylus input to keep the drawn line close to the
physical stylus in spite of input latency. Since the predictions are not exact, the line must be moved
from the predicted position to the actual position later on once it is known. Another example of
this technique is in VR APIs that predict head motion to reduce head tracking latency and the nausea
it causes.

Of course, not all forms of user input can be predicted. And predictions can be wrong, which becomes
more likely as the input latency you are trying to compensate for becomes larger. The Dear ImGui examples
do not currently implement any form of user input prediction.

# Platform-specific considerations

## Windows

DWM is the compositing window manager for Windows. For every application running in a window, DWM adds one
whole frame of input latency, as described in the [tearing prevention section](#tearing-prevention) above.
This is bad! DWM has some features that can remove this extra frame of latency, but only in a couple of
situations:

1. If you make a full screen window, and there are no overlay windows on top of yours (e.g. on-screen
  volume display), DWM will skip compositing and remove that frame of latency for you. There is no need
  to use the old full-screen exclusive mode for latency reasons anymore. (You also don't need to use
  full-screen exclusive mode to change the screen resolution, because you can use hardware scaling via
  DXGI instead. And if the display supports VRR then you don't need to change the refresh rate either,
  making full-screen exclusive mode entirely obsolete.)
1. For non-fullscreen windows, if the GPU driver supports a feature called Multiplane Overlay, then DWM
  can promote your window to a hardware overlay which removes that frame of latency. Multiplane Overlay
  is currently (in 2022) only supported on Intel GPUs, or Nvidia RTX 20 series or newer. And it is disabled
  if some features such as display rotation or 10 bit color are used.

To know if Multiplane Overlay is being used, and for exact input latency numbers, it is highly recommended
to use [PresentMon](https://github.com/GameTechDev/PresentMon) or one of the debugging tools that
integrate it such as PIX. Multiplane Overlay usage is shown in PresentMon as
"Hardware Composed: Independent Flip". PresentMon also shows an exact latency number which is calculated
as the time from the Present call on the CPU to the VSync when the pixels start being sent to the monitor.
(This is slightly less than the application's total input latency because it doesn't include the time from
input event processing to the Present call, but it's still a very valuable number to have.)

As an interesting bit of trivia, the software rendered mouse cursor hack is built into the Windows
operating system. Whenever you drag a window, DWM switches to a software rendered mouse cursor so that you
don't notice DWM's 1 frame of latency. But this only works for top-level windows managed by DWM. If you
want the same for your Dear ImGui windows you will have to use the io.MouseDrawCursor hack.

### DirectX

The DirectX 9 example has about 3 frames of latency. DirectX 9 cannot take advantage of Multiplane Overlay
and has poor control over queue depth. It is best to avoid DirectX 9 if you care about input latency for
windowed applications.

The DirectX 10, 11, and 12 examples use DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT and
SetMaximumFrameLatency to limit the queue depth. If you need higher GPU utilization or more protection
against missed frames you can increase the queue depth, at the cost of adding input latency.

When Multiplane Overlay is used, the DirectX 10, 11, and 12 examples achieve slightly less than one frame
of input latency. Otherwise they will achieve slightly less than 2 frames of input latency thanks to DWM's
added extra frame. It is possible to further reduce input latency using the other techniques above.

When Multiplane Overlay or fullscreen are used, it is possible to disable tearing prevention with
DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING. This is actually required to take advantage of
[VRR displays](#variable-refresh-rate-vrr). It also enables you to implement beam racing, if you really
want to.

### OpenGL

OpenGL does not support using the Multiplane Overlay feature, so non-fullscreen OpenGL windows will always
have more than one frame of latency, even if VSync is disabled. The Dear ImGui OpenGL samples should have
slightly less than 2 frames of input latency.

### Vulkan

TODO

## Apple

TODO

## Linux

The situation on Linux depends very much on your window manager, and whether you are using X11 or Wayland,
and whether you are using OpenGL or Vulkan, and what GPU driver you are using.

TODO: More investigation is needed.

## Android

The Android Game Development Kit provides a library called Swappy that is almost certainly what you want
to use to control your frame rate and queue depth. It has good documentation
[available here](https://developer.android.com/games/sdk/frame-pacing/).

Unfortunately Android's compositing window manager SurfaceFlinger may add a frame of latency, just as
other compositing window managers do.

TODO: Investigate if SurfaceFlinger latency is mitigated with hardware overlays, as on Windows.

TODO: Investigate tools for measuring latency on Android, similar to PresentMon on Windows.

## Web

Unfortunately WebGL and requestAnimationFrame give you no explicit control over the queue depth. I 
have done some experimentation with the "desynchronized=true" WebGL context attribute, pointerrawupdate,
and OffscreenCanvas in Chrome, and I was unable to wring any latency benefit out of these features, at
least on my Windows machine. Even if there is some benefit to be had it is likely to be Chrome only,
and not on all Chrome platforms, and a lot of work to implement, and fragile as browsers update and change
their behavior.

Unless the web platform adds explicit control over queue depth, input latency of WebGL and WebGPU content
in browsers is going to remain poor. Many of the other techniques discussed here are not possible on the
Web either.

## GLFW

TODO

## SDL

TODO