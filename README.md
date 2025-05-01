# xytherOS: Where Imagination Meets Execution

![POSIX Badge](https://img.shields.io/badge/POSIX-Compliant-brightgreen?style=flat&label=POSIX)
![Issues Tracking Badge](https://img.shields.io/badge/issue_track-github-blue?style=flat&label=Issue%20Tracking)
 [![Contributors](https://img.shields.io/github/contributors/emment-yamikani/xytheros)](https://github.com/emment-yamikani/xytheros/graphs/contributors)
![Commit activity](https://img.shields.io/github/commit-activity/m/emment-yamikani/xytheros)
![GitHub top language](https://img.shields.io/github/languages/top/emment-yamikani/xytheros?logo=c&label=)
[![GitHub license](https://img.shields.io/github/license/emment-yamikani/xytheros)](https://github.com/emment-yamikani/xytheros/blob/LICENSE)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/5ff3601b45194f9fbe54b2f8b3f380b0)](https://app.codacy.com/gh/emment-Yamikani/xytherOS/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade)

<center>
   <img width="300" height="200" src="images/logo.svg">
</center>

Welcome to xytherOS, a cutting-edge operating system designed for developers, hobbyists, and OS enthusiasts who seek a balance between creativity and technical excellence. xytherOS is built with flexibility, performance, and innovation in mind, providing a robust platform for experimentation and execution.

## Features

- **Lightweight and Efficient** – Optimized for performance with minimal overhead.
- **Custom Bootloader with GRUB Support** – Leverages GRUB to provide a seamless boot experience.
- **ELF File Format Compatibility** – Ensures smooth execution of binaries.
- **Modular and Extensible** – Designed with a modular architecture to allow easy expansion.
- **Developer-Friendly** – Provides tools and documentation to support OS development.

## Getting Started

To get started with xytherOS, follow these steps:

1. Clone the repository:

   ```sh
   git clone https://github.com/emment-yamikani/xytheros.git
   ```

2. Build the OS using the latest GNU cross compiler toolchain:

   **NOTE: Before building**

      Ensure you are using the latest version of GCC cross compiler and Binutils. Preferably greater than gcc-14.2.0 && binutils-2.44. Run the shell script:

      **This was tested on Manjaro Linux; So it may be different on your host OS.**

      ```sh
      ./build_cross.sh
      ```

      This will build and install gcc-14.2.0 and binutils-2.44. For different versions please modify `build_cross.sh` with respective versions greater than the defaults.

      ```sh
      make all
      ```

3. Run xytherOS in an emulator (such as QEMU):

   ```sh
   make run
   ```

## Contribution

We welcome contributions! If you're interested in improving xytherOS, please follow these guidelines:

- Fork the repository and create a feature branch.
- Submit a pull request with a detailed description of your changes.
- Follow the coding standards and document your code properly.

## License

xytherOS is released under the MIT license. See `LICENSE` for more details.

## Contact

For discussions, issues, and feature requests, feel free to open an issue or reach out to the community.

---
xytherOS: Where Imagination Meets Execution.
