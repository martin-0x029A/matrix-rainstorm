# Matrix Rain 🌧

A Matrix-style rain animation written in C, now packed with enhanced physics effects and fractal lightning. Optimized for WebAssembly (WASM) and powered by SDL2, this project is perfect for creating an engaging background effect on your website.

## 🎮 Preview

Check out the live demo: [Matrix Rain Effect](https://matrix-rain-d7z.pages.dev/matrix_rain)

## ✨ Features

- **Optimal Performance**: Written in C and compiled to WebAssembly for an optimal and responsive experience.
- **Dynamic Rain Physics**: Realistic falling columns influenced by gravity, terminal velocity, and adaptive wind effects.
- **Fractal Lightning Effects**: Enjoy stunning fractal lightning bolts that dynamically illuminate the rain with glowing, thick lines and realistic branching.
- **Smooth Trail Effects**: Uses an offscreen render target with translucent fading to create an immersive trail effect.
- **Diverse Unicode Character Set**: Features a wide range of characters including Japanese (Hiragana, Katakana), Latin, Cyrillic, Greek, mathematical symbols, and more. Easily customizable to fit your design.
- **WebGL2 Support**: Compiled with WebGL2 enabled for enhanced rendering capabilities on modern browsers.

## 🚀 Getting Started

### Prerequisites

- Python (for running a local development server)
- Emscripten compiler (for modifications and building)
- SDL2 and SDL2_ttf libraries (managed via Emscripten options)

### Running the Project

1. Start a local HTTP server:
   ```bash
   python -m http.server
   ```

2. Open `matrix_rain.html` in your browser.
3. Enjoy the enhanced Matrix rain effect with dynamic lightning and realistic physics!

## 🛠 Development

### Modifying and Compiling

To customize and recompile the project, use the following compile command:

```sh:README.md
emcc matrix_rain.c -O2 -s USE_SDL=2 -s USE_SDL_TTF=2 -s USE_WEBGL2=1 \
  --shell-file minimal.html \
  --preload-file matrix_font_subset.ttf \
  -o matrix_rain.html
```

This command compiles the code with WebGL2 support, ensuring improved graphics performance on modern browsers.

### Character Set & Font Customization

The default version includes a diverse subset of Unicode characters. To expand or customize the character set:

1. Download [Noto Sans CJK](https://github.com/googlefonts/noto-cjk/raw/main/Sans/Variable/OTC/NotoSansMonoCJK-VF.ttf.ttc)
2. Create a font subset that includes only the characters you need to keep the file size optimal.
3. Replace `matrix_font_subset.ttf` with your custom font in the compile command.

## 📝 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## ⚠️ Disclaimer

This code is provided as-is, without any warranties. Use it at your own risk.

## 🤝 Contributing

Contributions are welcome! Feel free to submit issues and pull requests.