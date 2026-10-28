#include "PianoRoll.h"

void PianoRoll::setProject(Project* newProject)
{
    project = newProject;
    repaint();
}

void PianoRoll::edit(const std::function<void()>& change)
{
    if (lock != nullptr)
    {
        juce::ScopedLock sl(*lock);
        change();
    }
    else
        change();
    repaint();
    if (onChanged)
        onChanged();
}

int PianoRoll::noteAt(juce::Point<int> position) const
{
    if (project == nullptr || project->selectedTrack < 0 || project->selectedTrack >= (int) project->tracks.size())
        return -1;
    const auto& notes = project->tracks[(size_t) project->selectedTrack].notes;
    for (int i = (int) notes.size() - 1; i >= 0; --i)
    {
        auto box = juce::Rectangle<int>((int) (notes[(size_t) i].startBeat * pixelsPerBeat) + keyWidth,
                                        (127 - notes[(size_t) i].note - lowestNote) * rowHeight,
                                        juce::jmax(6, (int) (notes[(size_t) i].lengthBeats * pixelsPerBeat)),
                                        rowHeight - 1);
        if (box.contains(position))
            return i;
    }
    return -1;
}

void PianoRoll::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff141618));
    for (int row = 0; row < 36; ++row)
    {
        const int note = lowestNote + (35 - row);
        auto bounds = juce::Rectangle<int>(0, row * rowHeight, getWidth(), rowHeight);
        const bool black = juce::MidiMessage::isMidiNoteBlack(note);
        g.setColour(black ? juce::Colour(0xff1b1e22) : juce::Colour(0xff22262b));
        g.fillRect(bounds);
        g.setColour(juce::Colour(0xff101214));
        g.fillRect(0, row * rowHeight, keyWidth, rowHeight);
        g.setColour(juce::Colour(0xffc5c9ce));
        g.setFont(11.0f);
        if (note % 12 == 0)
            g.drawText("C" + juce::String(note / 12 - 1), 4, row * rowHeight, keyWidth - 6, rowHeight, juce::Justification::centredLeft);
    }

    if (project == nullptr || project->selectedTrack < 0 || project->selectedTrack >= (int) project->tracks.size())
        return;

    const auto& track = project->tracks[(size_t) project->selectedTrack];
    for (int i = 0; i < (int) track.notes.size(); ++i)
    {
        const auto& note = track.notes[(size_t) i];
        const int row = 35 - (note.note - lowestNote);
        if (row < 0 || row >= 36)
            continue;
        auto box = juce::Rectangle<float>((float) (note.startBeat * pixelsPerBeat + keyWidth),
                                          (float) row * rowHeight + 1.0f,
                                          (float) juce::jmax(6.0, note.lengthBeats * pixelsPerBeat),
                                          (float) rowHeight - 2.0f);
        g.setColour(i == project->selectedNote ? juce::Colour(0xffffd166) : track.colour);
        g.fillRoundedRectangle(box, 3.0f);
    }
}

void PianoRoll::mouseDown(const juce::MouseEvent& event)
{
    grabKeyboardFocus();
    if (project == nullptr || project->selectedTrack < 0 || project->selectedTrack >= (int) project->tracks.size())
        return;

    const int existing = noteAt(event.getPosition());
    if (event.mods.isPopupMenu() && existing >= 0)
    {
        edit([this, existing] { project->tracks[(size_t) project->selectedTrack].notes.erase(project->tracks[(size_t) project->selectedTrack].notes.begin() + existing); });
        project->selectedNote = -1;
        return;
    }

    if (existing >= 0)
    {
        project->selectedNote = existing;
        dragNote = existing;
        dragOrigin = project->tracks[(size_t) project->selectedTrack].notes[(size_t) existing].startBeat;
        repaint();
        return;
    }

    if (event.x <= keyWidth)
        return;

    const int row = juce::jlimit(0, 35, event.y / rowHeight);
    MidiNote note;
    note.note = juce::jlimit(0, 127, lowestNote + (35 - row));
    note.startBeat = juce::jmax(0.0, std::round(((event.x - keyWidth) / pixelsPerBeat) * 4.0) / 4.0);
    note.lengthBeats = 1.0;
    edit([this, note] { project->tracks[(size_t) project->selectedTrack].notes.push_back(note); });
    project->selectedNote = (int) project->tracks[(size_t) project->selectedTrack].notes.size() - 1;
}

void PianoRoll::mouseDrag(const juce::MouseEvent& event)
{
    if (dragNote < 0 || project == nullptr)
        return;
    auto& notes = project->tracks[(size_t) project->selectedTrack].notes;
    if (dragNote >= (int) notes.size())
        return;
    const double start = juce::jmax(0.0, std::round((dragOrigin + event.getDistanceFromDragStartX() / pixelsPerBeat) * 4.0) / 4.0);
    edit([this, start] { project->tracks[(size_t) project->selectedTrack].notes[(size_t) dragNote].startBeat = start; });
}

void PianoRoll::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    if (event.mods.isShiftDown())
        lowestNote = juce::jlimit(0, 96, lowestNote + (wheel.deltaY > 0 ? 1 : -1));
    else
        pixelsPerBeat = juce::jlimit(16.0, 160.0, pixelsPerBeat * (wheel.deltaY > 0 ? 1.1 : 0.9));
    repaint();
}
