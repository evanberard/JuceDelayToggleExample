/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
DelayExampleAudioProcessor::DelayExampleAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                        #if ! JucePlugin_IsMidiEffect
                        #if ! JucePlugin_IsSynth
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        #endif
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                        #endif
                        ), parameters(*this, nullptr, juce::Identifier("DelayExampleValueTree"), {
                           
                        // Set up of the value tree
                        std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "DELAY_TOGGLE", 1 }, "Delay Toggle", true),
                        
})
                                     
#endif
{
    delayToggle = parameters.getRawParameterValue ("DELAY_TOGGLE");
}

DelayExampleAudioProcessor::~DelayExampleAudioProcessor()
{
}

//==============================================================================
const juce::String DelayExampleAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool DelayExampleAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool DelayExampleAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool DelayExampleAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double DelayExampleAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int DelayExampleAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int DelayExampleAudioProcessor::getCurrentProgram()
{
    return 0;
}

void DelayExampleAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String DelayExampleAudioProcessor::getProgramName (int index)
{
    return {};
}

void DelayExampleAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void DelayExampleAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumInputChannels();
    
    delay.prepare(spec);
    delay.reset();
    delay.setMaxDelayTime (1.0f);
    delay.setDelayTime    (0, 0.4f);
    delay.setDelayTime    (1, 0.4f);
    delay.setWetLevel     (1.0f);
    delay.setFeedback     (0.75f);
    
    delayToggleSmoother.reset(sampleRate, 0.025f);
    delayToggleSmoother.setCurrentAndTargetValue(0.0f);
}

void DelayExampleAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool DelayExampleAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void DelayExampleAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    auto numSamples = buffer.getNumSamples();
    
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    // Main DSP process:
    
    delayToggleSmoother.setTargetValue(*delayToggle);

    // Delay Buffer
    if (delayBuffer.getNumSamples() != numSamples) {
        delayBuffer.setSize(totalNumInputChannels, numSamples);
        delayBuffer.clear();
    }
  
    for (int channel = 0; channel < totalNumInputChannels; ++channel) {
        auto* channelData = buffer.getWritePointer(channel);
        delayBuffer.copyFrom(channel, 0, channelData, numSamples, 1);
    }
    
    for (int sample = 0; sample < numSamples; ++sample) {
        float gainToApply = delayToggleSmoother.getNextValue();
        delayBuffer.applyGain(sample, 1, gainToApply);
        buffer.applyGain(sample, 1, 1.0f - gainToApply);
    }
    
    juce::dsp::AudioBlock<float> delayBlock (delayBuffer);
    delay.process (juce::dsp::ProcessContextReplacing<float> (delayBlock));
    
    for (int channel = 0; channel < totalNumInputChannels; ++channel) {
        auto* channelData = delayBuffer.getWritePointer(channel);
        buffer.addFrom(channel, 0, channelData, numSamples);
    }
}

//==============================================================================
bool DelayExampleAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* DelayExampleAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
}

//==============================================================================
void DelayExampleAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void DelayExampleAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DelayExampleAudioProcessor();
}
