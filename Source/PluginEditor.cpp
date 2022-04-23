/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

void LookAndFeel::drawRotarySlider(juce::Graphics &g,
                                   int x, int y, int width, int height,
                                   float sliderPosProportional,
                                   float rotaryStartAngle,
                                   float rotaryEndAngle,
                                   juce::Slider &slider)
{
    using namespace juce;
    
    // Get our drawing bounds
    auto bounds = Rectangle<float>(x, y, width, height);
    auto center = bounds.getCentre();
    
    //  Knob Body
    // Draw a filled ellipse, filling the bounds
    g.setColour(Colour(20u, 20u, 30u));
    g.fillEllipse(bounds);
    // Draw a border for the ellipse
    g.setColour(Colour(0u, 0u, 0u));
    g.drawEllipse(bounds, 1.f);

    //  Knob Tick Mark
    // Create Rectangle
    Path path;
    Rectangle<float> rect;
    rect.setLeft(center.getX() - 2);
    rect.setRight(center.getX() + 2);
    rect.setTop(bounds.getY());
    rect.setBottom(center.getY());
    path.addRectangle(rect);
    // Now rotate it
    jassert(rotaryStartAngle < rotaryEndAngle);
    auto sliderAngleRad = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);
    path.applyTransform(AffineTransform().rotated(sliderAngleRad, center.getX(), center.getY()));
    // Draw it
    g.fillPath(path);
}

//==============================================================================

void RotarySliderWithLabels::paint(juce::Graphics &g)
{
    using namespace juce;
    
    // Set rotary range/bounds (uses RADIANS)
    auto startAngle = degreesToRadians(180.f + 45.f);
    auto endAngle = degreesToRadians(180.f - 45.f) + MathConstants<float>::twoPi;
    // Get the rotary slider's range and bounds
    auto range = getRange();
    auto bounds = getSliderBounds();
    
    // the jmap method here turns our slider's current value into a normalized value
    getLookAndFeel().drawRotarySlider(g,
                                      bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(),
                                      jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0),
                                      startAngle,
                                      endAngle,
                                      *this);
}

juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const
{
    return getLocalBounds();
}

//==============================================================================

ResponseCurve::ResponseCurve(_3BandEQAudioProcessor& audioProcessor) : audioProcessor(audioProcessor)
{
    // Tell our Listener to listen to the main audio processor chain parameters
    const auto& parameters = audioProcessor.getParameters();
    for (auto parameter : parameters)
    {
        parameter->addListener(this);
    }
    
    // Start the timer, update GUI at 60Hz refresh rate
    startTimerHz(60);
}

ResponseCurve::~ResponseCurve()
{
    // Tell our Listener to stop listening to the main audio processor chain parameters
    const auto& parameters = audioProcessor.getParameters();
    for (auto parameter : parameters)
    {
        parameter->removeListener(this);
    }
}

void ResponseCurve::parameterValueChanged(int parameterIndex, float newValue)
{
    // raise our atomic flag
    parametersChanged.set(true);
}

void ResponseCurve::timerCallback()
{
    // if parameters have been changed since the last timer tick...
    // ...lower the flag and update the Editor mono chain
    if (parametersChanged.compareAndSetBool(false, true))
    {
        auto chainSettings = getChainSettings(audioProcessor.APVTS);
        // update peak filter
        auto peakCoefficients = makePeakFilter(chainSettings, audioProcessor.getSampleRate());
        updateCoefficients(monoChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
        // update low cut filter
        auto lowCutCoefficients = makeLowCutFilter(chainSettings, audioProcessor.getSampleRate());
        updateCutFilter(monoChain.get<ChainPositions::LowCut>(), lowCutCoefficients, chainSettings.lowCutSlope);
        // update high cut filter
        auto highCutCoefficients = makeHighCutFilter(chainSettings, audioProcessor.getSampleRate());
        updateCutFilter(monoChain.get<ChainPositions::HighCut>(), highCutCoefficients, chainSettings.highCutSlope);
        
        // repaint GUI
        repaint();
    }
}

void ResponseCurve::paint (juce::Graphics& g)
{
    using namespace juce;
    
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (Colours::tan);
    
    auto responseArea = getLocalBounds();
    auto width = responseArea.getWidth();
    
    auto& peakFilter = monoChain.get<ChainPositions::Peak>();
    auto& lowCutFilter = monoChain.get<ChainPositions::LowCut>();
    auto& highCutFilter = monoChain.get<ChainPositions::HighCut>();
    
    auto sampleRate = audioProcessor.getSampleRate();
    
    // Magnitudes as doubles representing Gain (multiplicative)
    std::vector<double> magnitudes;
    // One magnitude value per pixel within response area width
    magnitudes.resize(width);
    // Calculate the magnitude at each pixel
    for (int i=0; i<width; i++)
    {
        // calculate corresponding frequency for this pixel
        auto freq = mapToLog10(double(i) / double(width), 20.0, 20000.0);
        
        // start with unity gain
        double magnitude = 1.f;
        // multiply the magnitude value by the resulting gain at this frequency...
        // ...from each non-bypassed filter in our processing chain
        // Peak Filter
        if (! monoChain.isBypassed<ChainPositions::Peak>())
            magnitude *= peakFilter.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        // Low Cut Filter
        if (! lowCutFilter.isBypassed<0>())
            magnitude *= lowCutFilter.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (! lowCutFilter.isBypassed<1>())
            magnitude *= lowCutFilter.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (! lowCutFilter.isBypassed<2>())
            magnitude *= lowCutFilter.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (! lowCutFilter.isBypassed<3>())
            magnitude *= lowCutFilter.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        // High Cut Filter
        if (! highCutFilter.isBypassed<0>())
            magnitude *= highCutFilter.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (! highCutFilter.isBypassed<1>())
            magnitude *= highCutFilter.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (! highCutFilter.isBypassed<2>())
            magnitude *= highCutFilter.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (! highCutFilter.isBypassed<3>())
            magnitude *= highCutFilter.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        
        // Convert gain to decibels
        magnitudes[i] = Decibels::gainToDecibels(magnitude);
        
        // Declare our response curve
        Path responseCurve;
        
        const double yMin = responseArea.getBottom();
        const double yMax = responseArea.getY();
        // function that converts decibels to screen coordinates
        auto map = [yMin, yMax](double input)
        {
            return jmap(input, -24.0, 24.0, yMin, yMax);
        };
        
        responseCurve.startNewSubPath( responseArea.getX(), map(magnitudes.front()) );
        
        for (size_t i=1; i<magnitudes.size(); i++)
        {
            responseCurve.lineTo( responseArea.getX() + i, map(magnitudes[i]) );
        }
        
        // draw rounded rectangle border.
        // second argument is corner size. third argument is line thickness
        g.setColour(Colours::orange);
        g.drawRoundedRectangle(responseArea.toFloat(), 4.f, 1.f);
        
        // draw response path
        // second argument is line thickness
        g.setColour(Colours::white);
        g.strokePath(responseCurve, PathStrokeType(2.f));
    }
}

//==============================================================================
//  Class Definition
//==============================================================================

_3BandEQAudioProcessorEditor::_3BandEQAudioProcessorEditor (_3BandEQAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
peakFreqSlider(*audioProcessor.APVTS.getParameter("Peak_Freq"), "Hz"),
peakGainSlider(*audioProcessor.APVTS.getParameter("Peak_Gain"), "dB"),
peakQSlider(*audioProcessor.APVTS.getParameter("Peak_Q"), ""),
lowCutFreqSlider(*audioProcessor.APVTS.getParameter("LowCut_Freq"), "Hz"),
lowCutSlopeSlider(*audioProcessor.APVTS.getParameter("LowCut_Slope"), "dB/oct"),
highCutFreqSlider(*audioProcessor.APVTS.getParameter("HighCut_Freq"), "Hz"),
highCutSlopeSlider(*audioProcessor.APVTS.getParameter("HighCut_Slope"), "dB/oct"),
responseCurve(audioProcessor),
peakFreqSliderAttachment(audioProcessor.APVTS, "Peak_Freq", peakFreqSlider),
peakGainSliderAttachment(audioProcessor.APVTS, "Peak_Gain", peakGainSlider),
peakQSliderAttachment(audioProcessor.APVTS, "Peak_Q", peakQSlider),
lowCutFreqSliderAttachment(audioProcessor.APVTS, "LowCut_Freq", lowCutFreqSlider),
lowCutSlopeSliderAttachment(audioProcessor.APVTS, "LowCut_Slope", lowCutSlopeSlider),
highCutFreqSliderAttachment(audioProcessor.APVTS, "HighCut_Freq", highCutFreqSlider),
highCutSlopeSliderAttachment(audioProcessor.APVTS, "HighCut_Slope", highCutSlopeSlider)
{
    // Add our GUI components
    for (auto* component : getComponents())
    {
        addAndMakeVisible(component);
    }
    
    // Set the plugin window size
    setSize (600, 400);
}

_3BandEQAudioProcessorEditor::~_3BandEQAudioProcessorEditor() {}

//==============================================================================
// Member functions
//==============================================================================

void _3BandEQAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace juce;
    
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (Colours::tan);
}

// GUI bounds/sizing methods
void _3BandEQAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    
    auto bounds = getLocalBounds();
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.33);
    responseCurve.setBounds(responseArea);
    
    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.33);
    auto highCutArea = bounds.removeFromRight(bounds.getWidth() * 0.5);
    
    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * 0.5));
    lowCutSlopeSlider.setBounds(lowCutArea);
    
    highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * 0.5));
    highCutSlopeSlider.setBounds(highCutArea);
    
    peakFreqSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.33));
    peakGainSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.5));
    peakQSlider.setBounds(bounds);
}

// Returns a vector containing all juce components in our AudioProcessorEditor
std::vector<juce::Component*> _3BandEQAudioProcessorEditor::getComponents()
{
    return
    {
        &peakFreqSlider,
        &peakGainSlider,
        &peakQSlider,
        &lowCutFreqSlider,
        &lowCutSlopeSlider,
        &highCutFreqSlider,
        &highCutSlopeSlider,
        &responseCurve
    };
}
