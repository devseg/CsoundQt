/*
    Copyright (C) 2010 Andres Cabrera
    mantaraya36@gmail.com

    This file is part of QuteCsound.

    QuteCsound is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    QuteCsound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#ifndef CSOUNDENGINE_H
#define CSOUNDENGINE_H

#include <QtCore>

#include "cwindow.h" // Necessary for WINDAT struct
#include <csound.hpp>
#include <csPerfThread.hpp>

#include    <sndfile.hh>
#include "types.h"
#include "csoundoptions.h"

class ConsoleWidget;
class Curve;

// Csound 5.10 needs to be destroyed for opcodes like ficlose to flush the output
// TODO is this still necessary?
#define QUTECSOUND_DESTROY_CSOUND

class CsoundEngine;
class WidgetLayout;

struct CsoundUserData {
  int result; //result of csoundCompile()
  CSOUND *csound; // instance of csound
  CsoundPerformanceThread *perfThread;
  CsoundEngine *cs; // Pass engine
  WidgetLayout *wl; // Pass widgets
  /* current configuration */
  MYFLT zerodBFS; //0dBFS value
  int numChnls;
  int sampleRate;
  long outputBufferSize;
  /* performance */
  // PERF_STATUS stores performance state when run in the same thread. Should not be used when threaded.
  int PERF_STATUS; //0=stopped 1=running
  MYFLT* outputBuffer;

  bool enableWidgets; // Whether widget values are processed in the callback
  bool threaded; // Whether running in a separate thread or not
  bool useInvalue; // To select between invalue/outvalue and chnget/chnset

  QVector<QString> channelNames;
  QVector<double> values;
  QVector<QString> stringValues;
  QVector<double> mouseValues;
  RingBuffer audioOutputBuffer;
  unsigned long ksmpscount;
};

class CsoundEngine : public QObject
{
  public:
    CsoundEngine();
    ~CsoundEngine();
    static void messageCallbackNoThread(CSOUND *csound,
                                         int attr,
                                         const char *fmt,
                                         va_list args);
    static void messageCallbackThread(CSOUND *csound,
                                         int attr,
                                         const char *fmt,
                                         va_list args);
    static void outputValueCallbackThread (CSOUND *csound,
                                    const char *channelName,
                                    MYFLT value);
    static void inputValueCallbackThread (CSOUND *csound,
                                   const char *channelName,
                                   MYFLT *value);
    static void outputValueCallback (CSOUND *csound,
                                    const char *channelName,
                                    MYFLT value);
    static void inputValueCallback (CSOUND *csound,
                                   const char *channelName,
                                   MYFLT *value);

    static void makeGraphCallback(CSOUND *csound, WINDAT *windat, const char *name);
    static void drawGraphCallback(CSOUND *csound, WINDAT *windat);
    static void killGraphCallback(CSOUND *csound, WINDAT *windat);
    static int exitGraphCallback(CSOUND *csound);
//     static void ioCallback (CSOUND *csound,
//                             const char *channelName,
//                             MYFLT *value,
//                             int channelType);
    static int keyEventCallback(void *userData,
                                void *p,
                                unsigned int type);
    static void csThread(void *data);  //Thread function (called after each performance pass by the performance thread)

    static void readWidgetValues(CsoundUserData *ud);
    static void writeWidgetValues(CsoundUserData *ud);

    void setCsoundOptions(const CsoundOptions &options);
    // Options unsafe to change while running
    void setThreaded(bool threaded);
    // Options safe to change while running
    void useInvalue(bool use);
    void enableWidgets(bool enable);

    void registerConsole(ConsoleWidget *c);
    void unregisterConsole(ConsoleWidget *c);
    void setConsoleBufferSize(int size);
    void keyPressForCsound(QString key);  // For key press events from consoles and widget panel
    void keyReleaseForCsound(QString key);
    int popKeyPressEvent();
    int popKeyReleaseEvent();

    void processEventQueue();
    void queueOutValue(QString channelName, double value);
    void queueOutString(QString channelName, QString value);
    void queueMessage(QString message);
    void clearMessageQueue();

    bool isRunning();

    QMutex perfMutex;  // TODO is this still needed?

  public slots:
    int play();
    void stop();
    void pause();
    void startRecording(int format, QString filename);
    void stopRecording();
    void queueEvent(QString eventLine, int delay);

  private:
    int runCsound(bool useAPI);
    void stopCsound();
    void dispatchQueues();
    QStack<Curve *> newCurveBuffer;  // To store curves from Csound for widget panel Graph widgets
    QVector<WINDAT *> curveBuffer;

    CsoundUserData *ud;

    SndfileHandle *outfile;
    long samplesWritten;
    bool m_recording;
    MYFLT *recBuffer; // for temporary copy of Csound output buffer when recording to file
    int bufferSize; // size of the record buffer

    CsoundOptions m_options; // FIXME how to fill these?
    // Options which are not safe to pass while running are stored in these
    // variables to pass on next run.
    bool m_threaded;


    MYFLT *pFields; // array of pfields for score and rt events

    QVector<ConsoleWidget *> consoles;
    int m_consoleBufferSize;
    QMutex messageMutex; // Protection for message queue
    QStringList messageQueue;  // Messages from Csound execution
    QMutex keyMutex; // For keys pressed to pass to Csound from console and widget panel
    QStringList keyPressBuffer; // protected by keyMutex
    QStringList keyReleaseBuffer; // protected by keyMutex

    QMutex eventMutex;
    QVector<QString> eventQueue;
    QTimer queueTimer;
    int refreshTime; // time in milliseconds for widget value updates (both input and output)
    QVector<unsigned long> eventTimeStamps;
    int eventQueueSize;

  private slots:
    void recordBuffer();

//  signals:
};

#endif // CSOUNDENGINE_H
