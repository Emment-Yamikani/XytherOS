# XytherOS: Where Imagination Meets Execution

   ![Qemu Screenshot](kernel/xytherOs_logo.png)

Welcome to XytherOS, a cutting-edge operating system designed for developers, hobbyists, and OS enthusiasts who seek a balance between creativity and technical excellence. XytherOS is built with flexibility, performance, and innovation in mind, providing a robust platform for experimentation and execution.

## Features
- **Lightweight and Efficient** – Optimized for performance with minimal overhead.
- **Custom Bootloader with GRUB Support** – Leverages GRUB to provide a seamless boot experience.
- **ELF File Format Compatibility** – Ensures smooth execution of binaries.
- **Modular and Extensible** – Designed with a modular architecture to allow easy expansion.
- **Developer-Friendly** – Provides tools and documentation to support OS development.

## Getting Started
To get started with XytherOS, follow these steps:
1. Clone the repository:
   ```sh
   git clone https://github.com/emment-yamikani/xytheros.git
   ```
2. Build the OS using the latest GNU cross compiler toolchain:
   ### NOTE: Before building.
      Ensure you are using the latest version of GCC cross compiler and Binutils. Preferably greater than gcc-14.2.0 && binutils-2.44. Run the shell script:

      **This was tested on Manjaro Linux; So it may be different on your host OS.**

      ```sh
      ./build_cross.sh
      ```

      This will build and install gcc-14.2.0 and binutils-2.44. For different versions please modify `build_cross.sh` with respective versions greater than the defaults.
      
      ```sh
      make all
      ```
3. Run XytherOS in an emulator (such as QEMU):
   ```sh
   make run
   ```

## Contribution
We welcome contributions! If you're interested in improving XytherOS, please follow these guidelines:
- Fork the repository and create a feature branch.
- Submit a pull request with a detailed description of your changes.
- Follow the coding standards and document your code properly.

## License
XytherOS is released under the MIT license. See `LICENSE` for more details.

## Contact
For discussions, issues, and feature requests, feel free to open an issue or reach out to the community.

---
XytherOS: Where Imagination Meets Execution.

