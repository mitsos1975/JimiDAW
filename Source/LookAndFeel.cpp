#include "LookAndFeel.h"

DawLookAndFeel::DawLookAndFeel()
{
    setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xff141414));
    setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2a2a));
    setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff3a3a3a));
    setColour(juce::TextButton::textColourOffId, juce::Colour(0xffd6d6d6));
    setColour(juce::TextButton::textColourOnId, juce::Colour(0xfff2f2f2));
    setColour(juce::Label::textColourId, juce::Colour(0xffd0d0d0));
    setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::thumbColourId, juce::Colour(0xffc8c8c8));
    setColour(juce::Slider::trackColourId, juce::Colour(0xff3c3c3c));
    setColour(juce::Slider::backgroundColourId, juce::Colour(0xff0c0c0c));
    setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffd6d6d6));
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff1a1a1a));
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff3a3a3a));
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff222222));
    setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff3a3a3a));
    setColour(juce::ComboBox::textColourId, juce::Colour(0xffd6d6d6));
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff1c1c1c));
    setColour(juce::PopupMenu::textColourId, juce::Colour(0xffe0e0e0));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff3a4a5c));
}

void DawLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                          const juce::Colour& backgroundColour,
                                          bool highlighted, bool down)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    const bool on = button.getToggleState();
    auto face = backgroundColour;
    if (on)
        face = face.brighter(0.18f);
    if (highlighted)
        face = face.brighter(0.08f);
    if (down)
        face = face.darker(0.22f);

    g.setColour(juce::Colour(0xff0a0a0a));
    g.fillRect(bounds);
    g.setColour(face);
    g.fillRect(bounds.reduced(1.0f));
    g.setColour(juce::Colours::white.withAlpha(down ? 0.02f : 0.07f));
    g.fillRect(bounds.reduced(1.0f).removeFromTop(bounds.getHeight() * 0.45f));
    g.setColour(juce::Colour(0xff000000).withAlpha(0.65f));
    g.drawRect(bounds, 1.0f);
}

void DawLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
                                    bool, bool down)
{
    auto area = button.getLocalBounds().toFloat().reduced(6.0f);
    const auto text = button.getButtonText();
    auto colour = button.findColour(button.getToggleState() ? juce::TextButton::textColourOnId
                                                            : juce::TextButton::textColourOffId);
    if (!button.isEnabled())
        colour = colour.withAlpha(0.4f);

    auto icon = [&](const juce::String& name)
    {
        g.setColour(name == "Record" ? juce::Colour(0xffc43636) : colour);
        auto box = area.withSizeKeepingCentre(juce::jmin(14.0f, area.getHeight() - 2.0f), juce::jmin(14.0f, area.getHeight() - 2.0f));
        if (down)
            box = box.translated(0.0f, 1.0f);
        if (name == "Play")
        {
            juce::Path triangle;
            triangle.addTriangle(box.getX(), box.getY(), box.getRight(), box.getCentreY(), box.getX(), box.getBottom());
            g.fillPath(triangle);
            return true;
        }
        if (name == "Pause")
        {
            g.fillRect(box.removeFromLeft(box.getWidth() * 0.35f));
            g.fillRect(box.removeFromRight(box.getWidth() * 0.55f));
            return true;
        }
        if (name == "Stop")
        {
            g.fillRect(box.reduced(1.0f));
            return true;
        }
        if (name == "Record")
        {
            g.fillEllipse(box.reduced(1.0f));
            return true;
        }
        return false;
    };

    if (icon(text))
        return;

    g.setColour(colour);
    g.setFont(juce::FontOptions(12.0f));
    g.drawFittedText(text, button.getLocalBounds().reduced(4, 1), juce::Justification::centred, 1);
}

void DawLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                     float sliderPos, float, float,
                                     juce::Slider::SliderStyle style, juce::Slider& slider)
{
    const bool vertical = style == juce::Slider::LinearVertical || style == juce::Slider::LinearBarVertical;
    auto bounds = juce::Rectangle<float>((float) x, (float) y, (float) width, (float) height);

    if (vertical)
    {
        auto slot = bounds.withSizeKeepingCentre(6.0f, bounds.getHeight() - 8.0f);
        g.setColour(juce::Colour(0xff070707));
        g.fillRoundedRectangle(slot.expanded(1.0f), 2.0f);
        g.setColour(juce::Colour(0xff2a2a2a));
        g.fillRoundedRectangle(slot, 1.5f);
        g.setColour(juce::Colour(0xff6ea8ff).withAlpha(0.85f));
        g.fillRect(slot.getX(), sliderPos, slot.getWidth(), slot.getBottom() - sliderPos);

        auto cap = juce::Rectangle<float>(bounds.getCentreX() - 11.0f, sliderPos - 8.0f, 22.0f, 16.0f);
        g.setColour(juce::Colour(0xff1a1a1a));
        g.fillRoundedRectangle(cap.translated(0.0f, 1.0f), 2.0f);
        juce::ColourGradient metal(juce::Colour(0xfff4f4f4), cap.getX(), cap.getY(),
                                   juce::Colour(0xff8e8e8e), cap.getX(), cap.getBottom(), false);
        g.setGradientFill(metal);
        g.fillRoundedRectangle(cap, 2.0f);
        g.setColour(juce::Colour(0xff4a4a4a));
        g.drawRoundedRectangle(cap, 2.0f, 1.0f);
        g.setColour(juce::Colour(0xff222222));
        g.drawLine(cap.getX() + 3.0f, cap.getCentreY(), cap.getRight() - 3.0f, cap.getCentreY(), 1.0f);
        return;
    }

    auto slot = bounds.withSizeKeepingCentre(bounds.getWidth() - 6.0f, 4.0f);
    g.setColour(juce::Colour(0xff070707));
    g.fillRoundedRectangle(slot.expanded(0.0f, 1.0f), 2.0f);
    g.setColour(juce::Colour(0xff8fb4ff));
    g.fillRect(slot.getX(), slot.getY(), juce::jmax(0.0f, sliderPos - slot.getX()), slot.getHeight());
    auto cap = juce::Rectangle<float>(sliderPos - 6.0f, bounds.getCentreY() - 8.0f, 12.0f, 16.0f);
    g.setColour(slider.findColour(juce::Slider::thumbColourId));
    g.fillRoundedRectangle(cap, 2.0f);
    juce::ignoreUnused(slider);
}

void DawLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                     float sliderPos, float startAngle, float endAngle, juce::Slider&)
{
    auto bounds = juce::Rectangle<float>((float) x, (float) y, (float) width, (float) height).reduced(4.0f);
    const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = startAngle + sliderPos * (endAngle - startAngle);

    g.setColour(juce::Colour(0xff050505));
    g.fillEllipse(bounds);
    juce::ColourGradient knob(juce::Colour(0xff5a5a5a), centre.x, centre.y - radius,
                              juce::Colour(0xff1c1c1c), centre.x, centre.y + radius, false);
    g.setGradientFill(knob);
    g.fillEllipse(bounds.reduced(2.0f));
    g.setColour(juce::Colour(0xff0a0a0a));
    g.drawEllipse(bounds.reduced(2.0f), 1.0f);

    juce::Path pointer;
    pointer.addRectangle(-1.2f, -radius + 5.0f, 2.4f, radius * 0.55f);
    g.setColour(juce::Colour(0xfff0f0f0));
    g.fillPath(pointer, juce::AffineTransform::rotation(angle).translated(centre.x, centre.y));
}
