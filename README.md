# Matrix Rain

This project is my very own Matrix Rain screen, written in C and optimized for WebAssembly (WASM) to run as a background on a website.

## Running the Project

To run the project, start a simple HTTP server (for example, using Python):

```
python -m http.server
```

Then, open the `matrix_rain.html` file in your browser.

## Modifications and Compilation

Feel free to modify the code and recompile it using the following command:

```sh:README.md
emcc matrix_rain.c -O2 -s USE_SDL=2 -s USE_SDL_TTF=2 \
  --shell-file minimal.html \
  --preload-file NotoSansMonoCJK-VF.ttf.ttc \
  -o matrix_rain.html
```

## Disclaimer

I'm not responsible for any damage caused by this code. Use it at your own risk.