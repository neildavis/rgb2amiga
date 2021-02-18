/*
*  Copyright (C) 2014-2021 Christophe Meneboeuf <christophe@xtof.info>
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include <string>
#include <iostream> 

#include <SDL.h>
#include "tclap/CmdLine.h"

#include "CError.h"

#include "CChunkyImage.h"
#include "CAmigaImage.h"

using namespace std;
using namespace Magick;


void SdlError(const std::string& error,
    SDL_Window* const pWindow = nullptr,
    SDL_Renderer* const pRenderer = nullptr
)
{
    std::cout << "Error: " << error << '\n';

    if (pRenderer != nullptr) {
      SDL_DestroyRenderer(pRenderer);
    }
    if (pWindow != nullptr) {
      SDL_DestroyWindow(pWindow);
    } 
    SDL_Quit();

    exit(1);
}

void DisplayPreview(const CChunkyImage& img, const int scale);

//**********************************//
// Required arguments:
// -i : input image
// -o : output file
//*********************************//
int main(int argc, char *argv[])
{
    try
    {
        //Parsing arguments
        TCLAP::CmdLine cmd{ "rgb2amiga - by Christophe Meneboeuf <christophe@xtof.info>", ' ', "1" };
        TCLAP::ValueArg<string> argInput{ "i", "input", "Input file to process.", true, "", "string" };
        TCLAP::ValueArg<string> argOutput("o", "output", "Output file.", true, "", "string");
        TCLAP::ValueArg<int>    argNbColors("c", "colors", "Number of colors to use. Defaults to \"32\".", false, 32, "string");
        TCLAP::ValueArg<string> argSize("s", "size", "Targeted size in WidthxHeight format. Defaults to \"320x256\"\n\tOptionnal suffix: '!' ignore the original aspect ratio. Only '!': keep input size", false, "320x256", "string");
        TCLAP::SwitchArg argDither("d", "dither", "Use dithering.");
        TCLAP::ValueArg<int> argPreview("p", "preview", "Open a window to display a scaled preview. Defaults to no preview.", false, 0, "scale");
        cmd.add(argInput);
        cmd.add(argOutput);
        cmd.add(argNbColors);
        cmd.add(argSize);
        cmd.add(argDither);        
        cmd.add(argPreview);
        cmd.parse( argc, argv );

        //Loading image 
        Image imgInput( argInput.getValue() );
        
        //Converting
        CPalette palette = CPaletteFactory::GetInstance().GetPalette("AMIGA");
        
        CChunkyImage chunkyImg;
        chunkyImg.Init(imgInput, argNbColors.getValue(), argDither.getValue(), palette, argSize.getValue());

        CAmigaImage amigaImg;
        amigaImg.Init(chunkyImg);
        amigaImg.Save(argOutput.getValue());

        std::cout << '\n' << argOutput.getValue() << " saved." << '\n';

        // Display the preview if requested
        if (argPreview.getValue()) {
          DisplayPreview(chunkyImg, argPreview.getValue());
        }
         
    }
    catch (TCLAP::ArgException &e)  // catch any exceptions
    {
        std::cerr << "Error: " << e.error() << " for arg " << e.argId() << std::endl;
        return 1;
    }
    catch (Magick::Exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    catch (CError& e)
    {
        std::cerr << "Error: " << e.What() << std::endl;
    }

    return 0;
}


void DisplayPreview(const CChunkyImage& img, const int scale)
{
  
  // init
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    SdlError("cannot initialise the preview window.");
  }
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0"); // nearest neighbor

  // window
  SDL_Window* pWindow = SDL_CreateWindow("Preview", SDL_WINDOWPOS_UNDEFINED,
    SDL_WINDOWPOS_UNDEFINED,
    img.GetWidth() * scale,
    img.GetHeight() * scale,
    SDL_WINDOW_SHOWN
  );
  if (pWindow == nullptr) {
    SdlError("cannot initialise the preview window.");
  }
  // renderer
  SDL_Renderer* pRenderer = SDL_CreateRenderer(pWindow, -1, SDL_RENDERER_ACCELERATED);
  if (pRenderer == nullptr) {
    SdlError("cannot initialise the preview window.", pWindow);
  }

  SDL_Texture* pTexture = SDL_CreateTexture(pRenderer, SDL_PIXELFORMAT_BGR888, SDL_TEXTUREACCESS_STATIC, img.GetWidth(), img.GetHeight());
  if (pTexture == nullptr) {
    SdlError("cannot initialise the preview window.", pWindow, pRenderer);
  }

  std::vector<rgba8Bits_t> buffer;
  const auto& indexes = img.GetPixels();
  const auto& palette = img.GetPalette();
  buffer.reserve(indexes.size());
  for (auto idx : indexes) {
    buffer.emplace_back(palette[idx]);
  }

  SDL_UpdateTexture(pTexture, nullptr, buffer.data(), sizeof(rgba8Bits_t) * img.GetWidth());
  SDL_RenderClear(pRenderer);
  SDL_RenderCopy(pRenderer, pTexture, NULL, NULL);
  SDL_RenderPresent(pRenderer);

  // event loop
  while (true)
  {
    SDL_Event e;
    if (SDL_WaitEvent(&e))
    {
      if (e.type == SDL_QUIT) { break; }
    }
  }

  SDL_DestroyTexture(pTexture);
  SDL_DestroyRenderer(pRenderer);
  SDL_DestroyWindow(pWindow);
  SDL_Quit();
}
