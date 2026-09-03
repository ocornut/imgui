# Security Policy

Dear ImGui is primarily designed for developer tools and technical applications running trusted code and trusted data. 

Given that the library was not designed as a hardened security boundary against hostile or intentionally malformed inputs: 
consider not using it in a context where a crash (through e.g. intentionally malformed inputs) could lead to privilege elevation. 
E.g. running in a privileged process and interacting with non-privileged clients feeding code or data to Dear ImGui.

Dear ImGui development efforts are focused on improving developer experience, usability, correctness, and robustness in real-world applications. 
We will do our best to reduce e.g. crashes caused by common programmer mistakes.

However, the project does not specifically focus on adversarial fuzzing scenarios, allocation-failure hardening, or highly artificial edge cases that are unlikely to occur in normal usage.

## Reporting a Vulnerability

Considering the above policy, most things may be reported in GitHub Issues.
If you have a reason to disclose privately, please reach out to the contact address listed in README.
