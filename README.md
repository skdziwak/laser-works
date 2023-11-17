# LaserWorks

LaserWorks is a software tool designed to convert SVG Paths into GCODE, suitable for CNC machines and 3D Printers.
## Features

- **SVG to GCODE Conversion:** Efficiently converts SVG paths into GCODE.
- **User-Friendly Interface:** Built using `gtkmm-3.0` for an intuitive user experience.
- **Metric Units:** Utilizes metric units for precise and standardized measurements.
- **Customizable Settings:** Allows configuration of machine-specific settings and preferences.

## Usage Guidelines

- Ensure that the SVG document size matches your machine's working area.
- Convert all SVG objects to paths for compatibility.
- Please note that LaserWorks currently does not support "arc" path commands.

## Getting Started

### Prerequisites

Before building LaserWorks, ensure you have the following installed:
- `gtkmm-3.0`
- `python3`
- `cmake` (version 3.16 or higher)

### Build Instructions

To build LaserWorks, run the following commands in your terminal:

```sh
cmake . -Bbuild
cd build
make
```

## Code Structure

### Main Components

- **Interface:** Manages user interactions and the graphical interface.
- **SVG Parsing:** Parses SVG files and transforms them into path elements.
- **Path Elements:** Handles different types of path elements like Line, CubicBezier, and QuadraticBezier.
- **GCODE Generation:** Transforms path elements into GCODE instructions.

## Contribution

Contributions to LaserWorks are welcome! Whether it's improving the code, fixing bugs, or enhancing documentation, your input is valued.

## License

> Copyright 2020 Szymon Dziwak
>
> Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
>
> The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
>
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
