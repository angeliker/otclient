/* The MIT License
 *
 * Copyright (c) 2010 OTClient, https://github.com/edubart/otclient
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


#include "fonts.h"
#include "font.h"
#include "resources.h"

Fonts g_fonts;

void Fonts::init()
{
    std::list<std::string> files = g_resources.getDirectoryFiles("fonts");
    foreach(const std::string& file, files) {
        if(boost::ends_with(file, ".yml")) {
            std::string name = file;
            boost::erase_first(name, ".yml");
            std::shared_ptr<Font> font(new Font);
            font->load("fonts/" + file);
            m_fonts[name] = font;
        }
    }
}

Font* Fonts::get(const std::string& fontName)
{
    auto it = m_fonts.find(fontName);
    if(it != m_fonts.end()) {
        return it->second.get();
    }
    logError("Font \"%s\" not found", fontName.c_str());
    return NULL;
}

Font *Fonts::getDefault()
{
    Font *font = get("tibia-10px-rounded");
    if(font) {
        return font;
    }
    logFatal("Default font not found!");
    return NULL;
}
