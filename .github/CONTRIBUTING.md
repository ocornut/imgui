## How to create an Issue

Hello! 

You may use the Issue tracker to submit bug reports, feature requests or suggestions. You may ask for help or advices as well, however please read this wall of text before doing so. Thank you!

**Prerequisites for new users of dear imgui:**
- Please read the FAQ in imgui.cpp.
- Please read misc/fonts/README.txt if your question relates to fonts or text.
- Please run ImGui::ShowDemoWindow() to explore the demo and its sources.
- Please use the Search function to look for similar issues. You may also browse issues by tags.
- If you get a assert, use a debugger to locate the line triggering it and read the comments around the assert.

**Guidelines to report an issue or ask a question:**
- Try to provide a Minimal, Complete and Verifiable Example ([MCVE](https://stackoverflow.com/help/mcve)) to demonstrate your problem. An ideal submission includes a small piece of code that anyone can paste in one of the examples/ application to understand and reproduce it. Narrowing your problem to its shortest and purest form is often the easiest way to understand it. Sometimes while creating the MCVE you will end up solving the problem!
- Try to attach screenshots to clarify the context. They often convey useful information that are omitted by the description. You can drag pictures/files here (prefer github attachments over 3rd party hosting).
- If you are discussing an assert or a crash, please provide a debugger callstack. 
- Please provide your imgui version number.
- Please state if you have made substancial modifications to your copy of imgui. 
- When discussing issues related to rendering or inputs, please state which OS you are using, and if you are using a vanilla copy of one of the back end (imgui_impl_xxx files), or modified one, or if you built your own.
- When requesting a new feature or pushing code, please describe the usage context (how you intend to use it, why you need it, etc.).

If you have been using dear imgui for a while and/or have been using C/C++ for several years and/or have demonstrated good behavior here, it is ok to not fullfill every item to the letter. Those are guidelines and experienced users of dear imgui will know what information are useful in a given context.

## How to create an Pull Request

- If you are adding a feature, please explain the context of the change: what do you need the feature for?
- Try to attach screenshots to clarify the context and demonstrate the feature at a glance.
- Make sure you create a branch for the pull request. In Git, 1 PR is associated to 1 branch. If you keep pushing to the same branch after you submitted the PR, your new commits will appear in the PR.
