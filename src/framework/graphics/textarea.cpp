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


#include "textarea.h"
#include "graphics.h"
#include "core/engine.h"

TextArea::TextArea() :
        m_font(0),
        m_align(ALIGN_TOP_LEFT),
        m_color(Color::white),
        m_cursorPos(-1),
        m_startRenderPos(0),
        m_cursorVisible(false)
{
}

TextArea::TextArea(Font* font,
                   const std::string& text,
                   const Rect& screenCoords,
                   int align,
                   const Color& color) :
        m_font(font),
        m_text(text),
        m_screenCoords(screenCoords),
        m_align(align),
        m_color(color),
        m_cursorPos(-1),
        m_startRenderPos(0),
        m_cursorVisible(false)
{
    recalculate();
}

void TextArea::draw()
{
    int numGlyphs = m_text.length();
    const TexturePtr& texture = m_font->getTexture();
    for(int i=0;i<numGlyphs;++i) {
        g_graphics.drawTexturedRect(m_glyphsCoords[i], texture, m_glyphsTexCoords[i], m_color);
    }

    // render cursor
    if(m_cursorVisible && m_cursorPos >= 0 && m_cursorPos <= numGlyphs) {
        const int delay = 500;
        int ticks = g_engine.getLastFrameTicks();
        // draw every 500ms
        if(ticks - m_cursorTicks <= delay) {
            Rect cursorRect;
            // when cursor is at 0 or is the first visible element
            if(m_cursorPos == 0 || (m_cursorPos < numGlyphs && m_glyphsCoords[m_cursorPos].topLeft() == m_drawArea.topLeft()))
                cursorRect = Rect(m_drawArea.left()-1, m_drawArea.top(), 1, m_font->getGlyphHeight());
            else
                cursorRect = Rect(m_glyphsCoords[m_cursorPos-1].right(), m_glyphsCoords[m_cursorPos-1].top(), 1, m_font->getGlyphHeight());
            g_graphics.drawFilledRect(cursorRect, m_color);
        } else if(ticks - m_cursorTicks >= 2*delay) {
            m_cursorTicks = g_engine.getLastFrameTicks();
        }
    }
}

void TextArea::recalculate()
{
    // prevent glitches from invalid rects
    if(!m_screenCoords.isValid())
        return;

    int textLenght = m_text.length();

    m_glyphsCoords.clear();
    m_glyphsTexCoords.clear();

    m_glyphsCoords.resize(textLenght);
    m_glyphsTexCoords.resize(textLenght);

    // map glyphs positions
    Size textBoxSize;
    std::vector<Point> glyphsPositions = m_font->calculateGlyphsPositions(m_text, m_align, &textBoxSize);
    const Rect *glyphsTextureCoords = m_font->getGlyphsTextureCoords();
    const Size *glyphsSize = m_font->getGlyphsSize();
    
    // adjust start view area when cursor is enabled and has text
    if(m_cursorPos >= 0 && textLenght > 0) {
        // adjust when cursor reachs left
        if(m_startRenderPos > m_cursorPos) {
            m_startInternalPos.x = glyphsPositions[m_cursorPos].x;
            m_startInternalPos.y = glyphsPositions[m_cursorPos].y - m_font->getTopMargin();
        // adjust when cursor reachs right
        } else if(m_cursorPos > m_startRenderPos) {
            Rect virtualRect(m_startInternalPos, m_screenCoords.size());
            int pos = m_cursorPos - 1; // element before cursor
            int glyph = (uchar)m_text[pos];
            Rect glyphRect(glyphsPositions[pos], glyphsSize[glyph]);
            
            // if the cursor is after the start render pos, then the glyph before the cursor must visible
            if(!virtualRect.contains(glyphRect.topLeft()) || !virtualRect.contains(glyphRect.bottomRight())) {
                m_startInternalPos.y = std::max(glyphRect.bottom() - virtualRect.height(), 0);
                m_startInternalPos.x = std::max(glyphRect.right() - virtualRect.width() + 1, 0);
            }
        }
    } else {
        m_startInternalPos = Point(0,0);
    }
    
    m_drawArea.setLeft(m_screenCoords.left());
    m_drawArea.setTop(m_screenCoords.top()+m_font->getTopMargin());
    m_drawArea.setRight(m_screenCoords.right());
    m_drawArea.setBottom(m_screenCoords.bottom());

    m_startRenderPos = -1;
    for(int i = 0; i < textLenght; ++i) {
        int glyph = (uchar)m_text[i];
        m_glyphsCoords[i] = Rect();

        // skip invalid glyphs
        if(glyph < 32)
            continue;

        // calculate initial glyph rect and texture coords
        Rect glyphScreenCoords(glyphsPositions[i], glyphsSize[glyph]);
        Rect glyphTextureCoords = glyphsTextureCoords[glyph];

        // first translate to align position
        if(m_align & ALIGN_BOTTOM) {
            glyphScreenCoords.translate(0, m_screenCoords.height() - textBoxSize.height());
        } else if(m_align & ALIGN_VERTICAL_CENTER) {
            glyphScreenCoords.translate(0, (m_screenCoords.height() - textBoxSize.height()) / 2);
        } else { // ALIGN_TOP
            // nothing to do
        }

        if(m_align & ALIGN_RIGHT) {
            glyphScreenCoords.translate(m_screenCoords.width() - textBoxSize.width(), 0);
        } else if(m_align & ALIGN_HORIZONTAL_CENTER) {
            glyphScreenCoords.translate((m_screenCoords.width() - textBoxSize.width()) / 2, 0);
        } else { // ALIGN_TOP
            // nothing to do
        }

        // only render glyphs that are after startRenderPosition
        if(glyphScreenCoords.bottom() < m_startInternalPos.y || glyphScreenCoords.right() < m_startInternalPos.x)
            continue;

        // bound glyph topLeft to startRenderPosition
        if(glyphScreenCoords.top() < m_startInternalPos.y) {
            glyphTextureCoords.setTop(glyphTextureCoords.top() + (m_startInternalPos.y - glyphScreenCoords.top()));
            glyphScreenCoords.setTop(m_startInternalPos.y);
        }
        if(glyphScreenCoords.left() < m_startInternalPos.x) {
            glyphTextureCoords.setLeft(glyphTextureCoords.left() + (m_startInternalPos.x - glyphScreenCoords.left()));
            glyphScreenCoords.setLeft(m_startInternalPos.x);
        }

        // subtract startInternalPos
        glyphScreenCoords.translate(-m_startInternalPos);

        // translate rect to screen coords
        glyphScreenCoords.translate(m_screenCoords.topLeft());

        // only render if glyph rect is visible on screenCoords
        if(!m_screenCoords.intersects(glyphScreenCoords))
            continue;

        // bound glyph bottomRight to screenCoords bottomRight
        if(glyphScreenCoords.bottom() > m_screenCoords.bottom()) {
            glyphTextureCoords.setBottom(glyphTextureCoords.bottom() + (m_screenCoords.bottom() - glyphScreenCoords.bottom()));
            glyphScreenCoords.setBottom(m_screenCoords.bottom());
        }
        if(glyphScreenCoords.right() > m_screenCoords.right()) {
            glyphTextureCoords.setRight(glyphTextureCoords.right() + (m_screenCoords.right() - glyphScreenCoords.right()));
            glyphScreenCoords.setRight(m_screenCoords.right());
        }

        // render glyph
        m_glyphsCoords[i] = glyphScreenCoords;
        m_glyphsTexCoords[i] = glyphTextureCoords;
        
        // set who was the first complete glyph rendered
        if(m_startRenderPos == -1 && glyphScreenCoords.size() == glyphsSize[glyph])
            m_startRenderPos = i;
    }
}

void TextArea::setFont(Font* font)
{
    m_font = font;
    recalculate();
}

void TextArea::setText(const std::string& text)
{
    m_text = text;
    if(m_cursorPos >= 0) {
        m_cursorPos = 0;
        m_cursorTicks = g_engine.getLastFrameTicks();
    }
    recalculate();
}

void TextArea::setScreenCoords(Rect screenCoords)
{
    m_screenCoords = screenCoords;
    recalculate();
}

void TextArea::setAlign(int align)
{
    m_align = align;
    recalculate();
}

void TextArea::enableCursor(bool enable)
{
    if(enable) {
        m_cursorPos = 0;
        m_cursorTicks = g_engine.getLastFrameTicks();
    } else
        m_cursorPos = -1;
    recalculate();
}

void TextArea::appendCharacter(char c)
{
    if(m_cursorPos >= 0) {
        std::string tmp;
        tmp = c;
        m_text.insert(m_cursorPos, tmp);
        m_cursorPos++;
        m_cursorTicks = g_engine.getLastFrameTicks();
        recalculate();
    }
}

void TextArea::removeCharacter(bool right)
{
    if(m_cursorPos >= 0) {
        if(right && (uint)m_cursorPos < m_text.length())
            m_text.erase(m_text.begin() + m_cursorPos);
        else if((uint)m_cursorPos <= m_text.length() && m_cursorPos > 0) {
            m_text.erase(m_text.begin() + (--m_cursorPos));
            m_cursorTicks = g_engine.getLastFrameTicks();
        }
        recalculate();
    }
}

void TextArea::moveCursor(bool right)
{
    if(right) {
        if((uint)m_cursorPos+1 <= m_text.length()) {
            m_cursorPos++;
            m_cursorTicks = g_engine.getLastFrameTicks();
        }
    } else {
        if(m_cursorPos-1 >= 0) {
            m_cursorPos--;
            m_cursorTicks = g_engine.getLastFrameTicks();
        }
    }
    recalculate();
}

int TextArea::getTextPos(Point pos)
{
    int textLength = m_text.length();
    dump << " get pos: " << pos << m_drawArea;
    for(int i=0;i<textLength;++i) {
        Rect clickGlyphRect = m_glyphsCoords[i];
        clickGlyphRect.addTop(m_font->getTopMargin() + m_font->getGlyphSpacing().height());
        clickGlyphRect.addLeft(m_font->getGlyphSpacing().width()+1);
        dump << clickGlyphRect; 
        if(m_glyphsCoords[i].contains(pos))
            return i;
    }
    return -1;
}
