# Matrix Rain 🌧

A mesmerizing Matrix-style rain animation written in C and optimized for WebAssembly (WASM). Perfect for creating an engaging background effect on your website.

## 🎮 Preview

Check out the live demo: [Matrix Rain Effect](https://matrix-rain-d7z.pages.dev/matrix_rain)

## ✨ Features

- Written in C for optimal performance
- Compiled to WebAssembly for web compatibility
- Customizable characters using CJK fonts
- Smooth animation with SDL2
- Lightweight and easy to integrate

## 🚀 Getting Started

### Prerequisites

- Python (for local development server)
- Emscripten compiler (for modifications)

### Running the Project

1. Start a local HTTP server:
   ```bash
   python -m http.server
   ```

2. Open `matrix_rain.html` in your browser
3. Enjoy the Matrix rain effect!

## 🛠 Development

### Modifying and Compiling

To customize and recompile the project:

```sh:README.md
emcc matrix_rain.c -O2 -s USE_SDL=2 -s USE_SDL_TTF=2 \
  --shell-file minimal.html \
  --preload-file matrix_font_subset.ttf \
  -o matrix_rain.html
```

The current version works only with the characters in the script. If you want to add more characters use Noto Sans CJK https://github.com/googlefonts/noto-cjk/raw/main/Sans/Variable/OTC/NotoSansMonoCJK-VF.ttf.ttc and compile only the ones you need to reduce the size of the file.

### Character Set Customization

The default version includes a subset of characters. To expand the character set:

1. Download [Noto Sans CJK](https://github.com/googlefonts/noto-cjk/raw/main/Sans/Variable/OTC/NotoSansMonoCJK-VF.ttf.ttc)
2. Select the characters you need
3. Create a subset font to maintain optimal file size

## 📝 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## ⚠️ Disclaimer

This code is provided as-is without any warranties. Use at your own risk.

## 🤝 Contributing

Contributions are welcome! Feel free to submit issues and pull requests.