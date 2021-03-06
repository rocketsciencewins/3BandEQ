/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <string>

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
    
    bool enabled = slider.isEnabled();
    
    //  Knob Body
    // Draw a filled ellipse, filling the bounds
    g.setColour(enabled ? Colour(70u, 70u, 80u) : Colours::dimgrey);
    g.fillEllipse(bounds);
    // Draw a border for the ellipse
    g.setColour(enabled ? Colour(50u, 50u, 60u) : Colours::darkgrey);
    g.drawEllipse(bounds, 1.f);
    
    // IF this slider is of our custom "with labels" type
    if (auto* rswl = dynamic_cast<RotarySliderWithLabels*>(&slider))
    {
        auto center = bounds.getCentre();
     
        //  Knob Tick Mark
        // Create Rectangle
        Path path;
        Rectangle<float> rect;
        rect.setLeft(center.getX() - 2);
        rect.setRight(center.getX() + 2);
        rect.setTop(bounds.getY());
        rect.setBottom(center.getY() - rswl->getTextHeight() * 1.5);
        path.addRoundedRectangle(rect, 2.f);
        // Now rotate it
        jassert(rotaryStartAngle < rotaryEndAngle);
        auto sliderAngleRad = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);
        path.applyTransform(AffineTransform().rotated(sliderAngleRad, center.getX(), center.getY()));
        // Draw it
        g.setColour(Colours::white);
        g.fillPath(path);
        
        //  Text Label
        // Font: Default type, height as defined by this RotarySliderWithLabels object
        g.setFont(rswl->getTextHeight());
        // Text: As defined by this RotarySliderWithLabels object
        auto text = rswl->getDisplayString();
        // Get string width of that text
        auto stringWidth = g.getCurrentFont().getStringWidth(text);
        // Draw a filled rectangle for our text label background.
        // Note: We're just reusing the Rectangle<float> object...
        // ...from the tick mark rather than define another.
        rect.setSize(stringWidth + 4, rswl->getTextHeight() + 2);
        rect.setCentre(center);
        g.setColour(Colour(0u, 0u, 0u));
        g.fillRect(rect);
        // Draw our text.
        // Note: drawFittedText method wants a Rectangle<int> so we have to convert here in-line
        g.setColour(Colours::white);
        g.drawFittedText(text, rect.toNearestInt(), juce::Justification::centred, 1);
    }
}

void LookAndFeel::drawToggleButton(juce::Graphics &g,
                                   juce::ToggleButton &toggleButton,
                                   bool isHighlighted,
                                   bool isPressed)
{
    using namespace juce;
    
    // If we can cast the passed toggle button object as our "PowerButton" type
    if ( auto* powerButton = dynamic_cast<PowerButton*>(&toggleButton))
    {
        Path powerButtonPath;
        
        // Define a square area that fits inside the local bounds.
        auto bounds = toggleButton.getLocalBounds();
        auto size = jmin(bounds.getWidth(), bounds.getHeight() - 6);    // find max size that can fit, shrink a bit
        auto rect = bounds.withSizeKeepingCentre(size, size).toFloat();
        
        // Angle from vertical where we want out power icon arc to end
        float angleDeg = 30.f;
        
        // Arc radius shrink in a bit. Don't fill square area completely.
        size -= 6;
        auto radius = size * 0.5f;
        
        // Define arc
        powerButtonPath.addCentredArc(rect.getCentreX(), rect.getCentreY(),     // center x and y
                                  radius, radius,                           // radius x and y
                                  0.f,                                      // rotation of ellipse
                                  degreesToRadians(angleDeg),               // radians start
                                  degreesToRadians(360.f - angleDeg),       // radians end
                                  true);                                    // start as new subpath?
        
        // Define line from true center to top center
        powerButtonPath.startNewSubPath(rect.getCentreX(), rect.getY());
        powerButtonPath.lineTo(rect.getCentre());
        
        // 2pt path stroke width, rounded corners
        PathStrokeType pathStrokeType(2.f, PathStrokeType::JointStyle::curved);
        
        // Use bypass state to determine color
        auto color = toggleButton.getToggleState() ? Colours::dimgrey : Colour(0u, 150u, 0u);
        g.setColour(color);
        
        // Draw power button icon
        g.strokePath(powerButtonPath, pathStrokeType);
        
        // Draw power button (circular) border
        g.drawEllipse(rect, 2.f);
    }
    // If passed toggle button object is NOT a "PowerButton", check if it's an "AnalyzerButton"
    else if ( auto* analyzerButton = dynamic_cast<AnalyzerButton*>(&toggleButton))
    {
        // Use bypass state to determine color (OPPOSITE logic of the PowerButton color)
        auto color = ! toggleButton.getToggleState() ? Colours::dimgrey : Colour(0u, 150u, 0u);
        g.setColour(color);
        
        auto bounds = toggleButton.getLocalBounds();
        g.drawRect(bounds);
        
        // Draw the path
        g.strokePath(analyzerButton->randomPath, PathStrokeType(1.f));
    }
}

//==============================================================================

void RotarySliderWithLabels::paint(juce::Graphics &g)
{
    using namespace juce;
    
    // Set rotary range (uses RADIANS)
    auto startAngle = degreesToRadians(180.f + 45.f);
    auto endAngle = degreesToRadians(180.f - 45.f) + MathConstants<float>::twoPi;
    // Get the rotary slider's range and bounds
    auto range = getRange();
    auto bounds = getSliderBounds();
    
//    // DEBUGGING: Draw a border around the local bounds area
//    g.setColour(Colours::orange);
//    g.drawRect(getLocalBounds());
    
    // Draw rotary slider
    // the jmap method here turns our slider's current value into a normalized value
    getLookAndFeel().drawRotarySlider(g,
                                      bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(),
                                      jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0),
                                      startAngle,
                                      endAngle,
                                      *this);
    
    // Draw labels
    auto center = bounds.toFloat().getCentre();
    auto radius = bounds.getWidth() * 0.5f;
    
    g.setColour( isEnabled() ? Colour(44u, 44u, 44u) : Colours::dimgrey);
    g.setFont(getTextHeight());
    
    auto numChoices = labels.size();
    for (int i=0; i<numChoices; i++)
    {
        auto pos = labels[i].position;
        // Just in case, make sure its normalized between 0 and 1
        jassert(0.f <= pos);
        jassert(pos <= 1.f);
        
        auto angle = jmap(pos, 0.f, 1.f, startAngle, endAngle);
        
        auto c = center.getPointOnCircumference(radius + getTextHeight() * 0.5f + 1, angle);
        
        Rectangle<float> rect;
        auto str = labels[i].label;
        rect.setSize(g.getCurrentFont().getStringWidth(str), getTextHeight());
        rect.setCentre(c);
        rect.setY(rect.getY() + getTextHeight());
        
        g.drawFittedText(str, rect.toNearestInt(), juce::Justification::centred, 1);
    }
}

// Define a rectangular area where the slider knob should be drawn
juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const
{
    // start by getting the overall local bounds for this slider
    auto bounds = getLocalBounds();
    // use the shorter dimension to define the knob size
    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());
    // shrink the size a bit to allow room for text
    size -= getTextHeight() * 2;
    // define a square area of that size for us to draw our knob in
    juce::Rectangle<int> rect;
    rect.setSize(size, size);
    // position that square just below the center of the local bounds
    rect.setCentre(bounds.getCentreX(), 0);
    rect.setY(2);
    
    return rect;
}

juce::String RotarySliderWithLabels::getDisplayString() const
{
    // just return this slider's value as a string
    //return juce::String(getValue());
    
    // If this rotary slider's attached RangedAudioParameter is a CHOICE parameter (e.g. Slope),
    // then return the name of the currently selected choice
    if (auto* choiceParameter = dynamic_cast<juce::AudioParameterChoice*>(rap))
        return choiceParameter->getCurrentChoiceName();
    
    // Create a string object for frequency labels so we can decide whether to display Hz or kHz
    juce::String str;
    bool addK = false;
    // Confirm that this object is a float parameter
    if (auto* floatParameter = dynamic_cast<juce::AudioParameterFloat*>(rap))
    {
        // If this freq value is 1000 or higher, display kHz
        float val = getValue();
        if (val > 999.f)
        {
            val /= 1000.f;
            addK = true;
        }
        // One decimal place if we're displaying kHz, otherwise none
        str = juce::String( val, (addK ? 1:0) );
    }
    else
    {
        // This rotary slider's attached parameter is NOT a float parameter.
        jassertfalse; // this should never happen!
    }
    
    // Return display string.
    // We need this check because Q has no units (no suffix displayString)
    if (suffix.isNotEmpty())
    {
        // add a space after the value
        str << " ";
        // add a 'k' if we're displaying kHz
        if (addK)
            str << "k";
        // add this RotarySliderWithLabels object's suffix string
        str << suffix;
    }
    
    return str;
}

//==============================================================================

ResponseCurve::ResponseCurve(_3BandEQAudioProcessor& audioProcessor) :
audioProcessor(audioProcessor),
leftChannelPathGenerator(audioProcessor.leftChannelFIFO),
rightChannelPathGenerator(audioProcessor.rightChannelFIFO)
{
    // Tell our Listener to listen to the main audio processor chain parameters
    const auto& parameters = audioProcessor.getParameters();
    for (auto parameter : parameters)
    {
        parameter->addListener(this);
    }
    
    // Update the response curve audio chain once to begin with
    updateChain();
    
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

void PathGenerator::process(juce::Rectangle<float> fftBounds, double sampleRate)
{
    // While there are buffers to pull,
    // if we can pull a buffer,
    // send it to the FFT data generator
    juce::AudioBuffer<float> tempIncomingBuffer;
    while ( leftChannelFIFO->getNumCompleteBuffersAvailable() > 0 )
    {
        if ( leftChannelFIFO->getAudioBuffer(tempIncomingBuffer) )
        {
            // Shift mono buffer over by the number of samples in tempIncomingBuffer
            auto numIncomingSamples = tempIncomingBuffer.getNumSamples();
            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, 0),
                                              monoBuffer.getReadPointer(0, numIncomingSamples),
                                              monoBuffer.getNumSamples() - numIncomingSamples);
            
            // Copy the samples from our tempIncomingBuffer to the end of our mono buffer
            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, monoBuffer.getNumSamples() - numIncomingSamples),
                                              tempIncomingBuffer.getReadPointer(0, 0),
                                              numIncomingSamples);
            
            // Send mono buffer to FFT data generator
            leftChannelFFTDataGenerator.produceFFTDataForRendering(monoBuffer, -48.f);
        }
    }
    
    // If there are FFT data buffers to pull, and we can pull a buffer,
    // generate a path
    const auto fftSize = leftChannelFFTDataGenerator.getFFTSize();
    // Bin width is...
    // 48000 samples per second / 2048 fft samples = 23 Hz
    const auto binWidth = sampleRate / (double)fftSize;
    
    while (leftChannelFFTDataGenerator.getNumAvailableFFTDataBlocks() > 0)
    {
        std::vector<float> fftData;
        if (leftChannelFFTDataGenerator.getFFTData(fftData))
        {
            pathGenerator.generatePath(fftData,
                                       fftBounds,
                                       fftSize,
                                       binWidth,
                                       -48.f);
        }
    }
    
    // While there are paths that can be pulled,
    //  pull as many as we can
    // Only display the most recent path
    while (pathGenerator.getNumPathsAvailable())
    {
        pathGenerator.getPath(leftChannelFFTPath);
    }
}

void ResponseCurve::timerCallback()
{
    // If analyzer is NOT bypassed,
    if ( isFFTAnalysisEnabled )
    {
        auto fftBounds = getAnalysisArea().toFloat();
        auto sampleRate = audioProcessor.getSampleRate();
        leftChannelPathGenerator.process(fftBounds, sampleRate);
        rightChannelPathGenerator.process(fftBounds, sampleRate);
    }
    
    // if parameters have been changed since the last timer tick...
    // ...lower the flag and update the Editor mono chain
    if (parametersChanged.compareAndSetBool(false, true))
    {
        // update the response curve's audio chain
        updateChain();
    }
    
    repaint();
}

void ResponseCurve::updateChain()
{
    auto chainSettings = getChainSettings(audioProcessor.APVTS);
    
    // update bypass settings
    monoChain.setBypassed<ChainPositions::LowCut>(chainSettings.lowCutBypass);
    monoChain.setBypassed<ChainPositions::HighCut>(chainSettings.highCutBypass);
    monoChain.setBypassed<ChainPositions::Peak>(chainSettings.peakBypass);
    // update peak filter
    auto peakCoefficients = makePeakFilter(chainSettings, audioProcessor.getSampleRate());
    updateCoefficients(monoChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
    // update low cut filter
    auto lowCutCoefficients = makeLowCutFilter(chainSettings, audioProcessor.getSampleRate());
    updateCutFilter(monoChain.get<ChainPositions::LowCut>(), lowCutCoefficients, chainSettings.lowCutSlope);
    // update high cut filter
    auto highCutCoefficients = makeHighCutFilter(chainSettings, audioProcessor.getSampleRate());
    updateCutFilter(monoChain.get<ChainPositions::HighCut>(), highCutCoefficients, chainSettings.highCutSlope);
}

void ResponseCurve::paint (juce::Graphics& g)
{
    using namespace juce;
    
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (Colours::tan);
    
    // Draw response curve area background grid
    g.drawImage(background, getLocalBounds().toFloat());
    
    auto responseArea = getAnalysisArea();
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
        if (! monoChain.isBypassed<ChainPositions::LowCut>() )
        {
            if (! lowCutFilter.isBypassed<0>())
                magnitude *= lowCutFilter.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (! lowCutFilter.isBypassed<1>())
                magnitude *= lowCutFilter.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (! lowCutFilter.isBypassed<2>())
                magnitude *= lowCutFilter.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (! lowCutFilter.isBypassed<3>())
                magnitude *= lowCutFilter.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        // High Cut Filter
        if (! monoChain.isBypassed<ChainPositions::HighCut>() )
        {
            if (! highCutFilter.isBypassed<0>())
                magnitude *= highCutFilter.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (! highCutFilter.isBypassed<1>())
                magnitude *= highCutFilter.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (! highCutFilter.isBypassed<2>())
                magnitude *= highCutFilter.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
            if (! highCutFilter.isBypassed<3>())
                magnitude *= highCutFilter.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        
        // Convert gain to decibels
        magnitudes[i] = Decibels::gainToDecibels(magnitude);
    }
    
    // Draw our response curve
    Path responseCurve;
    
    const double yMin = responseArea.getBottom();
    const double yMax = responseArea.getY();
    // converts decibels to screen coordinates
    auto map = [yMin, yMax](double input)
    {
        return jmap(input, -24.0, 24.0, yMin, yMax);
    };
    
    // Draw response curve
    responseCurve.startNewSubPath( responseArea.getX(), map(magnitudes.front()) );
    
    for (size_t i = 1; i < magnitudes.size(); i++)
    {
        responseCurve.lineTo( responseArea.getX() + i, map(magnitudes[i]) );
    }
    
    // If analyzer is NOT bypassed, draw the FFT analysis curve
    if ( isFFTAnalysisEnabled )
    {
        // Draw left channel FFT analyzer path
        auto leftChannelFFTPath = leftChannelPathGenerator.getPath();
        leftChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY()));
        g.setColour(Colours::brown);
        g.strokePath(leftChannelFFTPath, PathStrokeType(1.f));
        // Draw right channel FFT analyzer path
        auto rightChannelFFTPath = rightChannelPathGenerator.getPath();
        rightChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY()));
        g.setColour(Colours::maroon);
        g.strokePath(rightChannelFFTPath, PathStrokeType(1.f));
    }

    // draw rounded rectangle border.
    // second argument is corner size. third argument is line thickness
    g.setColour(Colours::orange);
    g.drawRoundedRectangle(getRenderArea().toFloat(), 4.f, 1.f);
    
    // draw response path
    // second argument is line thickness
    g.setColour(Colours::white);
    g.strokePath(responseCurve, PathStrokeType(2.f));
}

// Called when plugin is resized, and BEFORE paint.
void ResponseCurve::resized()
{
    using namespace juce;
    
    background = Image(Image::PixelFormat::RGB, getWidth(), getHeight(), true);
    Graphics g(background);
    
    // Frequency grid values
    Array<float> freqs
    {
        20, 30, 40, 50, 60, 70, 80, 90, 100,
        200, 300, 400, 500, 600, 700, 800, 900, 1000,
        2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000,
        20000
    };
    
    // Get render area info, cache it for the upcoming calcs
    auto renderArea = getAnalysisArea();
    auto left = renderArea.getX();
    auto right = renderArea.getRight();
    auto top = renderArea.getY();
    auto bottom = renderArea.getBottom();
    auto width = renderArea.getWidth();
    
    // Get associated x positions for each freq
    Array<float> xs;
    for (auto freq : freqs)
    {
        auto normalizedX = mapFromLog10(freq, 20.f, 20000.f);
        xs.add(left + width * normalizedX);
    }
    
    // Draw frequency grid vertical lines at each x position
    g.setColour(Colours::dimgrey);
    for (auto x : xs)
    {
        g.drawVerticalLine(x, top, bottom);
    }
    
    // Gain grid values (dB)
    Array<float> gains
    {
        -24, -12, 0, 12, 24
    };
    
    // Draw gain grid vertical lines
    for (auto gain : gains)
    {
        auto y = jmap(gain, -24.f, 24.f, float(bottom), float(top));
        // Draw 0dB gain line with our slider color. All others dark grey.
        g.setColour( gain == 0.f ? Colour(0u, 150u, 0u) : Colours::darkgrey );
        g.drawHorizontalLine(y, left, right);
    }
    
    // Prepare to draw grid line labels
    g.setColour(Colours::darkgrey);
    const int fontHeight = 10;
    g.setFont(fontHeight);
    
    // Draw frequency grid line labels
    for (int i=0; i<freqs.size(); i++)
    {
        auto freq = freqs[i];
        auto x = xs[i];
        
        // Get first digit of this freq...
        int digits = (int)log10(freq);
        int firstDigit = (int)(freq / pow(10, digits));
        
        // ...and only draw the label if the first digit is 1, 2, 3, or 5.
        if ( firstDigit == 1 || firstDigit == 2 || firstDigit == 3 || firstDigit == 5 )
        {
            // Add "k" if frequency is in kilohertz range
            bool addK = false;
            String str;
            if (freq > 999.f)
            {
                addK = true;
                freq /= 1000.f;
            }
            
            // Assemble label string
            str << freq;
            if (addK)
                str << "k";
            str << "Hz";
            
            // Define rectangle area to fit label text
            auto textWidth = g.getCurrentFont().getStringWidth(str);
            Rectangle<int> rect;
            rect.setSize(textWidth, fontHeight);
            rect.setCentre(x, 0);
            rect.setY(1);
            
            // Draw label text
            g.drawFittedText(str, rect, juce::Justification::centred, 1);
        }
    }
    
    // Draw gain grid line labels
    for (auto gain : gains)
    {
        auto y = jmap(gain, -24.f, 24.f, float(bottom), float(top));
        
        // Assemble label string
        String str;
        if (gain > 0)
            str << "+";
        str << gain;
        
        // Define rectangle area to fit label text
        auto textWidth = g.getCurrentFont().getStringWidth(str);
        Rectangle<int> rect;
        rect.setSize(textWidth, fontHeight);
        rect.setX(getWidth() - textWidth);
        rect.setCentre(rect.getCentreX(), y);
        
        // Draw label text... Green text at 0dB, otherwise dark grey
        g.setColour( gain == 0.f ? Colour(0u, 150u, 0u) : Colours::darkgrey );
        g.drawFittedText(str, rect, juce::Justification::centred, 1);
    }
}

juce::Rectangle<int> ResponseCurve::getRenderArea()
{
    auto bounds = getLocalBounds();
    
    bounds.removeFromTop(12);
    bounds.removeFromBottom(2);
    bounds.removeFromLeft(20);
    bounds.removeFromRight(20);
    
    return bounds;
}

juce::Rectangle<int> ResponseCurve::getAnalysisArea()
{
    auto bounds = getRenderArea();
    
    bounds.removeFromTop(4);
    bounds.removeFromBottom(4);
    
    return bounds;
}

//==============================================================================
//  Class Definition
//==============================================================================

_3BandEQAudioProcessorEditor::_3BandEQAudioProcessorEditor (_3BandEQAudioProcessor& p) :
AudioProcessorEditor (&p), audioProcessor (p),

responseCurve(audioProcessor),

peakFreqSlider(*audioProcessor.APVTS.getParameter("Peak_Freq"), "Hz"),
peakGainSlider(*audioProcessor.APVTS.getParameter("Peak_Gain"), "dB"),
peakQSlider(*audioProcessor.APVTS.getParameter("Peak_Q"), ""),
lowCutFreqSlider(*audioProcessor.APVTS.getParameter("LowCut_Freq"), "Hz"),
lowCutSlopeSlider(*audioProcessor.APVTS.getParameter("LowCut_Slope"), "dB/oct"),
highCutFreqSlider(*audioProcessor.APVTS.getParameter("HighCut_Freq"), "Hz"),
highCutSlopeSlider(*audioProcessor.APVTS.getParameter("HighCut_Slope"), "dB/oct"),

peakFreqSliderAttachment(audioProcessor.APVTS, "Peak_Freq", peakFreqSlider),
peakGainSliderAttachment(audioProcessor.APVTS, "Peak_Gain", peakGainSlider),
peakQSliderAttachment(audioProcessor.APVTS, "Peak_Q", peakQSlider),
lowCutFreqSliderAttachment(audioProcessor.APVTS, "LowCut_Freq", lowCutFreqSlider),
lowCutSlopeSliderAttachment(audioProcessor.APVTS, "LowCut_Slope", lowCutSlopeSlider),
highCutFreqSliderAttachment(audioProcessor.APVTS, "HighCut_Freq", highCutFreqSlider),
highCutSlopeSliderAttachment(audioProcessor.APVTS, "HighCut_Slope", highCutSlopeSlider),

lowCutBypassButtonAttachment(audioProcessor.APVTS, "LowCut_Bypass", lowCutBypassButton),
highCutBypassButtonAttachment(audioProcessor.APVTS, "HighCut_Bypass", highCutBypassButton),
peakBypassButtonAttachment(audioProcessor.APVTS, "Peak_Bypass", peakBypassButton),
analyzerBypassButtonAttachment(audioProcessor.APVTS, "Analyzer_Bypass", analyzerBypassButton)
{
    // Define min/max value labels for our rotary sliders
    peakFreqSlider.labels.add({0.f, "20 Hz"});
    peakFreqSlider.labels.add({1.f, "20 kHz"});
    peakGainSlider.labels.add({0.f, "-24 dB"});
    peakGainSlider.labels.add({1.f, "+24 dB"});
    peakQSlider.labels.add({0.f, "0.1"});
    peakQSlider.labels.add({1.f, "10.0"});
    lowCutFreqSlider.labels.add({0.f, "20 Hz"});
    lowCutFreqSlider.labels.add({1.f, "20 kHz"});
    lowCutSlopeSlider.labels.add({0.f, "12 dB/Oct"});
    lowCutSlopeSlider.labels.add({1.f, "48 dB/Oct"});
    highCutFreqSlider.labels.add({0.f, "20 Hz"});
    highCutFreqSlider.labels.add({1.f, "20 kHz"});
    highCutSlopeSlider.labels.add({0.f, "12 dB/Oct"});
    highCutSlopeSlider.labels.add({1.f, "48 dB/Oct"});

    // Add our GUI components
    for (auto* component : getComponents())
    {
        addAndMakeVisible(component);
    }
    
    // Set up bypass buttons' look and feel
    lowCutBypassButton.setLookAndFeel(&lookAndFeel);
    highCutBypassButton.setLookAndFeel(&lookAndFeel);
    peakBypassButton.setLookAndFeel(&lookAndFeel);
    analyzerBypassButton.setLookAndFeel(&lookAndFeel);
    
    // Set up bypass buttons onClick function:
    // Enable sliders for all non-bypassed filters in our chain, and vice-versa
    auto safePtr = juce::Component::SafePointer<_3BandEQAudioProcessorEditor>(this);
    lowCutBypassButton.onClick = [safePtr]()
    {
        if ( auto* juceComp = safePtr.getComponent() )
        {
            auto bypassed = juceComp->lowCutBypassButton.getToggleState();
            juceComp->lowCutFreqSlider.setEnabled( !bypassed );
            juceComp->lowCutSlopeSlider.setEnabled( !bypassed );
        }
    };
    highCutBypassButton.onClick = [safePtr]()
    {
        if ( auto* juceComp = safePtr.getComponent() )
        {
            auto bypassed = juceComp->highCutBypassButton.getToggleState();
            juceComp->highCutFreqSlider.setEnabled( !bypassed );
            juceComp->highCutSlopeSlider.setEnabled( !bypassed );
        }
    };
    peakBypassButton.onClick = [safePtr]()
    {
        if ( auto* juceComp = safePtr.getComponent() )
        {
            auto bypassed = juceComp->peakBypassButton.getToggleState();
            juceComp->peakFreqSlider.setEnabled( !bypassed );
            juceComp->peakGainSlider.setEnabled( !bypassed );
            juceComp->peakQSlider.setEnabled( !bypassed );
        }
    };
    // Same thing for analyzer
    analyzerBypassButton.onClick = [safePtr]()
    {
        if ( auto* juceComp = safePtr.getComponent() )
        {
            auto enabled = juceComp->analyzerBypassButton.getToggleState();
            juceComp->responseCurve.setFFTAnalysisEnabled( enabled );
        }
    };
    
    
    // Set the plugin window size
    setSize (600, 400);
}

_3BandEQAudioProcessorEditor::~_3BandEQAudioProcessorEditor()
{
    lowCutBypassButton.setLookAndFeel(nullptr);
    highCutBypassButton.setLookAndFeel(nullptr);
    peakBypassButton.setLookAndFeel(nullptr);
    
    analyzerBypassButton.setLookAndFeel(nullptr);
}

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
    
    auto analyzerBypassArea = bounds.removeFromTop(25);
    analyzerBypassArea.setWidth(100);
    analyzerBypassArea.setX(5);
    analyzerBypassArea.removeFromTop(2);
    analyzerBypassButton.setBounds(analyzerBypassArea);
    
    bounds.removeFromTop(5);
    
    float heightRatio = 25.f / 100.f;
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * heightRatio);
    responseCurve.setBounds(responseArea);
    
    // Space between the response area and the slider areas below it
    bounds.removeFromTop(10);
    
    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.33);
    auto highCutArea = bounds.removeFromRight(bounds.getWidth() * 0.5);
    
    lowCutBypassButton.setBounds(lowCutArea.removeFromTop(25));
    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * 0.5));
    lowCutSlopeSlider.setBounds(lowCutArea);
    
    highCutBypassButton.setBounds(highCutArea.removeFromTop(25));
    highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * 0.5));
    highCutSlopeSlider.setBounds(highCutArea);
    
    peakBypassButton.setBounds(bounds.removeFromTop(25));
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
        
        &responseCurve,
        
        &lowCutBypassButton,
        &highCutBypassButton,
        &peakBypassButton,
        &analyzerBypassButton
    };
}
