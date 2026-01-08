#pragma once
#include <JuceHeader.h>

class LampChoice : public juce::Component
{
public:
    using OnSelect = std::function<void(int index)>;

    LampChoice() = default;

    void setItems(juce::StringArray newItems)
    {
        items = std::move(newItems);
        repaint();
    }

    void setSelectedIndex(int idx)
    {
        selected = juce::jlimit(0, juce::jmax(0, items.size() - 1), idx);
        repaint();
    }

    int getSelectedIndex() const { return selected; }

    void setOnSelect(OnSelect cb) { onSelect = std::move(cb); }

    void setLampOnRight(bool v)
    {
        lampOnRight = v;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        g.setColour(juce::Colours::white.withAlpha(0.9f));
        g.setFont(14.0f);

        const int rowH = getRowHeight();
        const int lampR = 6;
        const int pad = 8;

        const int lampX = lampOnRight
            ? (getWidth() - pad - lampR)
            : (pad + lampR);

        const int textLeft = pad;
        const int textRight = pad;

        const int textX = lampOnRight
            ? textLeft
            : (pad + 2 * lampR + 10);

        const int textW = lampOnRight
            ? (getWidth() - (pad + 2 * lampR + 10) - textRight)
            : (getWidth() - textX - textRight);



        for (int i = 0; i < items.size(); ++i)
        {
            auto row = juce::Rectangle<int>(0, i * rowH, getWidth(), rowH);

            // hover подсветка
            if (i == hovered)
            {
                g.setColour(juce::Colours::white.withAlpha(0.06f));
                g.fillRect(row);
            }

            const int cy = row.getCentreY();

            // лампочка
            const bool on = (i == selected);

            // ободок
            g.setColour(juce::Colours::white.withAlpha(0.35f));
            g.drawEllipse((float)(lampX - lampR), (float)(cy - lampR), (float)(lampR * 2), (float)(lampR * 2), 1.0f);

            if (on)
            {
                // “свет” — просто заливка + лёгкое свечение
                g.setColour(juce::Colours::white.withAlpha(0.85f));
                g.fillEllipse((float)(lampX - lampR + 2), (float)(cy - lampR + 2), (float)((lampR - 2) * 2), (float)((lampR - 2) * 2));

                g.setColour(juce::Colours::white.withAlpha(0.12f));
                g.fillEllipse((float)(lampX - lampR - 3), (float)(cy - lampR - 3), (float)((lampR + 3) * 2), (float)((lampR + 3) * 2));
            }

            // текст
            g.setColour(juce::Colours::white.withAlpha(on ? 0.95f : 0.7f));
            g.drawText(items[i], textX, row.getY(), textW, rowH,
                       lampOnRight ? juce::Justification::centredRight
                                   : juce::Justification::centredLeft);

        }
    }

    void mouseMove(const juce::MouseEvent& e) override
    {
        const int i = rowIndexAt(e.position);
        if (i != hovered) { hovered = i; repaint(); }
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        hovered = -1;
        repaint();
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        const int i = rowIndexAt(e.position);
        if (i >= 0 && i < items.size())
        {
            setSelectedIndex(i);
            if (onSelect) onSelect(i);
        }
    }

    int getIdealHeight() const { return items.size() * getRowHeight(); }

private:
    int getRowHeight() const { return 24; }

    int rowIndexAt(juce::Point<float> p) const
    {
        const int i = (int) (p.y / (float)getRowHeight());
        return (i >= 0 && i < items.size()) ? i : -1;
    }

    juce::StringArray items;
    int selected = 0;
    int hovered = -1;
    OnSelect onSelect;

    bool lampOnRight = false;
};
