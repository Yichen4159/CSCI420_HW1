# README for OpenGL Height Field Renderer

## Program Features

This OpenGL application is designed to render height fields from image data interactively. It supports various rendering modes, including points, wireframe, solid triangles, and smoothed height fields with color grading. The core functionality ensures compliance with OpenGL Core Profile version 3.2 or higher, emphasizing shader-based rendering and modern OpenGL practices. Users can interact with the height field through keyboard and mouse inputs, allowing for rotation, translation, and scaling. Vertex coloring is based on height, enhancing the visual representation of terrain elevation.

## Extra Credit Implementations

- **Custom Shaders**: Implemented advanced shaders for material and lighting effects, enhancing the realism and visual appeal of the rendered scenes.
- **Element Arrays and glDrawElements**: Utilized for efficient rendering, significantly improving performance and rendering speeds.
- **Color Support in Input Images**: The program can process color images (3 bytes per pixel), allowing for richer input data and more detailed height fields.
- **Wireframe on Solid Triangles**: Achieved through `glPolygonOffset`, this feature adds depth and clarity to the rendered models without z-buffer artifacts.
- **Color-Based Vertex Coloring**: In addition to height-based coloring, vertices can be colored using values from a secondary image, providing flexibility in visual output.
- **Texture Mapping**: Supports applying arbitrary images as textures on the height field, offering extensive customization of the visual output.
- **JetColorMap Coloring**: When activated, colors the surface using the JetColorMap function in the vertex shader, converting grayscale to vibrant color mappings.

## Additional Notes

The application is structured to facilitate easy understanding and further development. Extensive comments throughout the codebase aid in navigating and comprehending the implemented features and rendering techniques.

To showcase the capabilities of this program, an animation sequence is created, demonstrating the dynamic and interactive rendering possibilities. This sequence, along with individual JPEG frames, highlights both the technical proficiency and artistic creativity employed in the project.