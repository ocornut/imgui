## How to create an Issue

Hello! 

You may use the Issue Tracker to submit bug reports, feature requests or suggestions. You may ask for help or advice as well. However please read this wall of text before doing so. The amount of incomplete or ambiguous requests due to people not following those guidelines is often overwhelming. Please do your best to clarify your request. Thank you!

**Prerequisites for new users of dear imgui:**
- Please read the FAQ in imgui.cpp.
- Please read misc/fonts/README.txt if your question relates to fonts or text.
- Please run ImGui::ShowDemoWindow() to explore the demo and its sources.
- Please use the Search function of GitHub to look for similar issues. You may also browse issues by tags.
- Please use the Search function of your IDE to search in the code for comments related to your situation.
- If you get a assert, use a debugger to locate the line triggering it and read the comments around the assert.

**Guidelines to report an issue or ask a question:**
- Please provide your imgui version number.
- If you are discussing an assert or a crash, please provide a debugger callstack. 
- Please state if you have made substantial modifications to your copy of imgui. 
- When discussing issues related to rendering or inputs, please state the OS/back-end/renderer you are using. Please state if you are using a vanilla copy of one of the back-end (imgui_impl_xxx files), or a modified one, or if you built your own.
- Please provide a Minimal, Complete and Verifiable Example ([MCVE](https://stackoverflow.com/help/mcve)) to demonstrate your problem. An ideal submission includes a small piece of code that anyone can paste in one of the examples/ application (e.g. in main.cpp or imgui_demo.cpp) to understand and reproduce it. Narrowing your problem to its shortest and purest form is the easiest way to understand it. Please test your shortened code to ensure it actually exhibit the problem. Often while creating the MCVE you will end up solving the problem! Many questions that are missing a standalone verifiable example are missing the actual cause of their issue in the description, which ends up wasting everyone's time.
- Try to attach screenshots to clarify the context. They often convey useful information that are omitted by the description. You can drag pictures/files here (prefer github attachments over 3rd party hosting).
- When requesting a new feature, please describe the usage context (how you intend to use it, why you need it, etc.).
- Due to frequent abuse of this service from a certain category of users, if your GitHub account is anonymous and was created five minutes ago please understand that your post will receive more scrutiny and less patience for incomplete questions.

If you have been using dear imgui for a while and/or have been using C/C++ for several years and/or have demonstrated good behavior here, it is ok to not fullfill every item to the letter. Those are guidelines and experienced users of dear imgui will know what information are useful in a given context.

## How to create an Pull Request

- If you are adding a feature, please describe the usage context (how you intend to use it, why you need it, etc.).
- Try to attach screenshots to clarify the context and demonstrate the feature at a glance. You can drag pictures/files here (prefer github attachments over 3rd party hosting).
- Make sure you create a branch for the pull request. In Git, 1 PR is associated to 1 branch. If you keep pushing to the same branch after you submitted the PR, your new commits will appear in the PR (we can still cherry-pick individual commits).
