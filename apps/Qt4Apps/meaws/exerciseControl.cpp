#include <iostream>
using namespace std;

#include "exerciseControl.h"

#include "PowerSpectrum.h"
#include "Power.h"

#include <sstream>

ExerciseControl::ExerciseControl() {
	displayPitches = NULL;
	displayAmplitudes = NULL;
	hopSize = 8;

	noteButton = new QToolButton*[5];
}

ExerciseControl::~ExerciseControl() {
	if (instructionImageLabel != NULL) {
		instructionArea->removeWidget(instructionImageLabel);
		delete (instructionImageLabel);
		instructionImageLabel = NULL;
	}
	if (displayPitches != NULL) {
		resultArea->removeWidget(displayPitches);
		delete displayPitches;
		displayPitches = NULL;
	}
	if (displayAmplitudes != NULL) {
		resultArea->removeWidget(displayAmplitudes);
		delete displayAmplitudes;
		displayAmplitudes = NULL;
	}
}

int ExerciseControl::getType() {
	return TYPE_CONTROL;
}

void ExerciseControl::setupDisplay() {
	displayPitches = new QtMarPlot();
	displayPitches->setPlotName("Pitch");
	displayPitches->setBackgroundColor(QColor(255,255,255));
	displayPitches->setPixelWidth(3);
	displayAmplitudes = new QtMarPlot();
	displayAmplitudes->setPlotName("Amplitude");
	displayAmplitudes->setBackgroundColor(QColor(255,255,255));
	displayAmplitudes->setPixelWidth(3);
	/*
	displayPitches = new QLabel;
	displayAmplitude = new QLabel;
	displayPitches->setText("Display Pitches here");
	displayAmplitude->setText("Display Ampitudes here");
	resultArea->addWidget(displayPitches,0,0);
	resultArea->addWidget(displayAmplitude,0,1);
	*/
	//QHBoxLayout *displayLayout = new QHBoxLayout;
	resultArea->addWidget(displayPitches,0,0);
	resultArea->addWidget(displayAmplitudes,0,1);
	//resultArea->addLayout(displayLayout);

	//	resultsDisplay = new MeawsDisplay();
	//	mainLayout->addLayout(resultsDisplay);
}

void ExerciseControl::open(QString exerciseFilename) {
	QString noteImageBaseFilename = exerciseFilename.remove(".png");
	QString noteImageFilename;
	QPixmap image;

	notes = new QButtonGroup;
	for (int i=0; i<5; i++) {
		noteButton[i] = new QToolButton;
		noteImageFilename = noteImageBaseFilename+"-"+QString::number(i+1)+".png";
		//cout<<qPrintable(noteButtonFilename)<<endl;
		image = QPixmap::fromImage(QImage( noteImageFilename ));
	    //noteButton[i]->setIcon(QPixmap::fromImage(QImage( noteImageFilename )));
		noteButton[i]->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	    noteButton[i]->setIcon( image );
		noteButton[i]->setIconSize( image.size() );
		//noteButton[i]->setScaledContents(false);
		noteButton[i]->setMaximumHeight(120);
		notes->addButton(noteButton[i]);
		notes->setId( noteButton[i],i+1);
		instructionArea->addWidget(noteButton[i],0,i,Qt::AlignLeft|Qt::AlignTop);
	}
//	instructionArea->addWidget(notes);
	connect(notes, SIGNAL(buttonClicked(int)), this, SLOT(setNote(int)));
}
//zz

void ExerciseControl::setNote(int noteNumber) {
	cout<<noteNumber<<endl;
}


QString ExerciseControl::exercisesDir() {
	QString toReturn(MEAWS_DIR);
	toReturn.append("data/control/");
	return toReturn;
}

QString ExerciseControl::getMessage() {
	QString toReturn(resultString.c_str());
	return toReturn;
}

bool ExerciseControl::displayAnalysis(MarBackend *results) {
	mrs_natural i, j=0;
	realvec tmpP = results->getPitches();
	realvec tmpA = results->getAmplitudes();
	myPitches.stretch(tmpP.getSize());
	myAmplitudes.stretch(tmpP.getSize());

	// remove zeros pitches
	for(i=0 ; i< tmpP.getSize() ; i++)
		if(tmpP(i))
		{
	myPitches(j) = tmpP(i);
	myAmplitudes(j++) = tmpA(i);
	}
		myPitches.stretch(j);
		myAmplitudes.stretch(j);

	myPitches.apply(hertz2bark);
	myWeight = myAmplitudes;
	myAmplitudes.apply(amplitude2dB);

	displayPitches->setVertical(myPitches.median()-0.04,myPitches.median()+0.02);
	displayPitches->setData( &myPitches );

	cout << myAmplitudes ;

	displayAmplitudes->setVertical(0,myAmplitudes.maxval()+1);
	displayAmplitudes->setData( &myAmplitudes);

	evaluatePerformance(results, straightMezzo);

	std::stringstream ss;
	ss << "Results: " << pitchError << " " << amplitudeError;
	resultString = ss.str();

	/*MATLAB_PUT(myPitches, "pitch");
	MATLAB_PUT(myAmplitudes, "amp");
    MATLAB_EVAL("subplot(2, 1, 1); plot(pitch); subplot(2, 1, 2); plot(amp); ");*/

	return 0 ;
}

void ExerciseControl::evaluatePerformance(MarBackend *results, exerciseControlType type) {
	switch(type)
	{
	case straightPiano:
		pitchError = evaluateStraight(myPitches, myWeight);
		amplitudeError = evaluateStraight(myAmplitudes, myWeight);
		break;
	case straightMezzo:
		pitchError = evaluateStraight(myPitches, myWeight);
		amplitudeError = evaluateStraight(myAmplitudes, myWeight);
		break;
	case straightForte:
		pitchError = evaluateStraight(myPitches, myWeight);
		amplitudeError = evaluateStraight(myAmplitudes, myWeight);
		break;
	case crescendoDecrescendo:
		pitchError = evaluateStraight(myPitches, myWeight);
		amplitudeError = evaluateCrescendoDecrescendo(myAmplitudes, myWeight);
		break;
	case vibrato:
		pitchError = evaluateVibrato(myPitches, myWeight);
		amplitudeError = evaluateVibrato(myAmplitudes, myWeight);
		break;
	}
}

mrs_real ExerciseControl::evaluateStraight(realvec &vec, realvec &weight)
{
	return slidingWeightedDeviation(vec, weight)/vec.mean();
}

mrs_real ExerciseControl::evaluateCrescendoDecrescendo(realvec &vec, realvec &weight)
{
	mrs_natural maxIndex;

	mrs_real vecMax = vec.maxval(&maxIndex);
	mrs_real slope1 = (vecMax-vec(0))/(maxIndex);
	mrs_real slope2 = (vecMax-vec(vec.getSize()-1))/(vec.getSize()-maxIndex);

	realvec vecLinear(vec.getSize());

	for (mrs_natural i=0 ; i<maxIndex ; i++)
		vecLinear(i) = slope1*(i+vec(0));
	for (mrs_natural i=maxIndex ; i<vec.getSize() ; i++)
		vecLinear(i) = slope2*(vec.getSize()-i)+vec(vec.getSize()-1);

	// TODO: compile fix, might be yucky.  -gp
	realvec vecSubtracted = vec - vecLinear;
	return slidingWeightedDeviation( vecSubtracted, weight)/(vec.maxval()-vec.minval());
}

mrs_real ExerciseControl::evaluateVibrato(realvec &vec, realvec &weight)
{
	realvec vibratoFrequency (vec.getSize()), vibratoWeight(vec.getSize());
	realvec window(4096), windowWeight(hopSize*2);
	realvec spectrum(4096), power(1);

	window.setval(0);

	if(vec.getSize() != weight.getSize())
	{
		MRSERR("weigtedDeviation computation, data and weight must have the same dimensions\n");
		return 0;
	}

	// create necesary marsystems for vibrato analysis
	PowerSpectrum ps("PowerSpectrum");
	ps.updctrl("mrs_natural/inSamples", window.getCols());
	ps.updctrl("mrs_natural/inObservations", window.getRows());
	ps.updctrl("mrs_natural/onSamples", spectrum.getCols());
	ps.updctrl("mrs_natural/onObservations", spectrum.getRows());

	Power p("Power");
	p.updctrl("mrs_natural/inSamples", windowWeight.getCols());
	p.updctrl("mrs_natural/inObservations", windowWeight.getRows());
	p.updctrl("mrs_natural/onSamples", power.getCols());
	p.updctrl("mrs_natural/onObservations", power.getRows());

	for (mrs_natural i=0 ; i<vec.getSize() ; i++)
	{
		mrs_real meanValue = 0 ;
		for (mrs_natural j=0 ; j<vec.getSize() ; j++)
			meanValue += vec(i+j);
		meanValue/=vec.getSize();

		for (mrs_natural j=0 ; j<vec.getSize() ; j++)
		{
			window(j) = vec(i+j)-meanValue;
			windowWeight(j) = weight(i+j);
		}
		ps.process(window, spectrum);
		p.process(windowWeight, power);

		mrs_natural indexMax=0;
		spectrum.maxval(&indexMax);
		vibratoFrequency(i) = indexMax/4096*44100;
		vibratoWeight(i) = power(0);
	}

	return slidingWeightedDeviation(vibratoFrequency, vibratoWeight);
}

mrs_real ExerciseControl::slidingWeightedDeviation(realvec &vec, realvec &weight)
{
	mrs_real res=0;
	realvec window(hopSize*2), windowWeight(hopSize*2);

	if(vec.getSize() != weight.getSize())
	{
		MRSERR("weigtedDeviation computation, data and weight must have the same dimensions\n");
		return 0;
	}

	for (mrs_natural i=0 ; i<vec.getSize()-hopSize*2 ; i+=hopSize)
	{
		for (mrs_natural j=0 ; j<hopSize*2 ; j++)
		{
			window(j) = vec(i+j);
			windowWeight(j) = weight(i+j);
		}
		res += weightedDeviation(window, windowWeight);
	}
	return res/vec.getSize()*hopSize;
}

mrs_real ExerciseControl::weightedDeviation(realvec &vec, realvec &weight)
{
	mrs_real meanData = vec.mean(), res=0;

	if(vec.getSize() != weight.getSize())
	{
		MRSERR("weigtedDeviation computation, data and weight must have the same dimensions\n");
		return 0;
	}

	for (mrs_natural i=0 ; i<vec.getSize() ; i++)
		res+= (vec(i)-meanData)*(vec(i)-meanData)*weight(i);

	return sqrt(res/(vec.getSize()*weight.mean()));
}
