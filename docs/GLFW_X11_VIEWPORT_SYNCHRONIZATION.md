# GLFW/X11 multi-viewport geometry synchronization

## Problem

Dear ImGui's GLFW backend drives secondary viewport geometry through
`glfwSetWindowPos()` and `glfwSetWindowSize()`. Those calls are asynchronous on
X11: the window manager may accept, adjust, or reject the request after GLFW has
returned.

The portable GLFW API does not expose a request identifier or an acknowledgement
containing the geometry accepted by the window manager. Its position and size
callbacks are change notifications, not completion notifications.

This matters for a completely rejected X11 `ConfigureRequest`. ICCCM-compliant
window managers send a synthetic `ConfigureNotify` containing the unchanged
geometry, but GLFW only invokes its public position and size callbacks when that
geometry differs from GLFW's cached state. GLFW consumes the synthetic event, so
the application receives no callback. Dear ImGui can consequently retain the
requested geometry while the native window remains at its previous geometry.
Rendering, hit testing, mouse coordinates, and subsequent drag requests then use
inconsistent origins.

This is best described as a GLFW API limitation rather than an unambiguous GLFW
bug: an unchanged position is not necessarily expected to produce a "position
changed" callback. However, the current API is insufficient for robust
requested-versus-observed window geometry synchronization on asynchronous window
systems.

## Why the obvious alternatives are insufficient

- **Immediate readback after a setter:** the window manager is a separate X11
  client. `glfwGetWindowPos()` may still return the old geometry before the
  window manager processes the request, even when the request will be accepted.
- **Readback after a fixed timeout:** reduces the race but does not eliminate it.
  It also introduces latency and frame-rate-, load-, and window-manager-dependent
  behavior.
- **Removing GLFW's setter-echo suppression:** regresses delayed position/size
  callback cases previously fixed for #2117 and #2756.
- **Observing GLFW's X11 connection:** GLFW owns and drains that event queue; the
  backend has no public native-event hook through which to observe consumed
  events.

## Current compatibility approach

The backend keeps the existing callback path and classifies a recent callback as
a harmless setter echo only when its integer geometry matches both the latest
issued setter and the geometry still expected by the viewport. Adjusted or stale
geometry requests an authoritative readback during the next Dear ImGui frame.

For the unchanged-rejection case, the X11 backend optionally opens a second X11
connection and selects `StructureNotifyMask` on secondary viewport windows. This
connection receives the synthetic `ConfigureNotify` that GLFW otherwise filters.
The backend uses it only as a signal to compare native and viewport geometry; the
normal GLFW getters remain the authoritative source.

Using a second X11 connection requires explicit cross-connection ordering during
window creation and destruction. It is deliberately optional: if the native X11
symbols or observer connection are unavailable, the backend falls back to the
ordinary GLFW callback behavior rather than failing initialization.

This avoids a timing heuristic, but it is compatibility machinery that would be
preferable to remove once supported GLFW versions expose an adequate event or
acknowledgement facility.

## Preferred GLFW-side capability

A long-term GLFW solution could provide any one of:

1. a configure-request completion notification, including unchanged rejection;
2. a native event hook that observes X11 events before GLFW filters them; or
3. documented delivery of synthetic `ConfigureNotify` through an additional
   callback whose semantics do not imply that geometry changed.

With such a capability, Dear ImGui could retain its portable requested/observed
state handling while removing the separate X11 observer for newer GLFW versions.

## Required regression coverage

The synchronization contract should cover:

- synchronous and delayed exact setter echoes;
- adjusted/clamped position and size;
- complete rejection with unchanged native geometry;
- two outstanding requests whose responses arrive in separate frames;
- simultaneous position and size changes;
- active dragging at screen boundaries without snap-back or setter oscillation;
- collapse/uncollapse and delayed-size behavior from #2117/#2756; and
- native Xorg with Mutter, since XWayland is useful but not equivalent evidence
  for this event path.

The Test Engine mock can deterministically validate Dear ImGui's core ownership
and drag-rebasing behavior. It cannot replace backend-level or native-X11 tests,
because `-viewport-mock` replaces the GLFW platform handlers and therefore does
not execute GLFW's callback filtering or the X11 observer.
