#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDir>
#include <QFileDialog>
#include <QShortcut>
#include <QInputDialog>
#include <QTextStream>

//#include "Framework/Interface.hpp"
#include "M68k/sim68000/m68000.hpp"
//#include "M68k/sim68360/cpu32.hpp"
#include "M68k/devices/DeviceRegistry.hpp"
#include "M68k/loader/Loader.hpp"
#include <memory>


MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow),
  interface(NULL), registers(new RegisterModelList),
  breakpoints(new BreakpointModelList),
  code(new CodeLineList(breakpoints)),
  memory(new MemoryModelList(this))
{
  memoryDock = new MemoryDockWidget(this, 0, false, this);
  addDockWidget(Qt::BottomDockWidgetArea, memoryDock);
  ui->setupUi(this);
  setTabPosition(Qt::BottomDockWidgetArea, QTabWidget::North);
  setTabPosition(Qt::RightDockWidgetArea, QTabWidget::West);
  setTabPosition(Qt::LeftDockWidgetArea, QTabWidget::East);
  //setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
  QMainWindow::tabifyDockWidget(ui->registerDock, ui->breakpointDock);
  QMainWindow::tabifyDockWidget(memoryDock, ui->consoleDock);
  //QMainWindow::tabifyDockWidget(ui->memoryDock, ui->portDockA);
  //QMainWindow::tabifyDockWidget(ui->memoryDock, ui->portDockB);
  QMainWindow::tabifyDockWidget(memoryDock, ui->traceDock);
  ui->registerDock->raise();
  memoryDock->raise();

  ui->menubar->addSeparator();
  stepAction = new QAction("Step", ui->menubar);
  connect(stepAction,SIGNAL(triggered()),this,SLOT(onStep()));
  stepAction->setStatusTip("Execute a single instruction (F7)");
  stepAction->setToolTip("Execute a single instruction (F7)");
  ui->menubar->addAction(stepAction);
  new QShortcut(QKeySequence(Qt::Key_F7), this, SLOT(onStep()));
  runAction = new QAction("Run", ui->menubar);
  connect(runAction,SIGNAL(triggered()),this,SLOT(onRun()));
  runAction->setStatusTip("Execute until breakpoint (F8)");
  runAction->setToolTip("Execute until breakpoint (F8)");
  ui->menubar->addAction(runAction);
  new QShortcut(QKeySequence(Qt::Key_F8), this, SLOT(onRun()));
  runStepAction = new QAction("RunStep", ui->menubar);
  connect(runStepAction,SIGNAL(triggered()),this,SLOT(onRunStep()));
  runStepAction->setStatusTip("Step until breakpoint (F9)");
  runStepAction->setToolTip("Step until breakpoint (F9)");
  ui->menubar->addAction(runStepAction);
  new QShortcut(QKeySequence(Qt::Key_F9), this, SLOT(onRunStep()));
  resetAction = new QAction("Reset", ui->menubar);
  connect(resetAction,SIGNAL(triggered()),this,SLOT(onReset()));
  resetAction->setStatusTip("Reset processor state (F10)");
  resetAction->setToolTip("Reset processor state (F10)");
  ui->menubar->addAction(resetAction);
  new QShortcut(QKeySequence(Qt::Key_F10), this, SLOT(onReset()));
  toggleBPAction = new QAction("ToggleBP", ui->menubar);
  connect(toggleBPAction,SIGNAL(triggered()),this,SLOT(onToggleBP()));
  toggleBPAction->setStatusTip("Toggle breakpoint in current instruction (F2)");
  toggleBPAction->setToolTip("Toggle breakpoint in current instruction (F2)");
  ui->menubar->addAction(toggleBPAction);
  new QShortcut(QKeySequence(Qt::Key_F2), this, SLOT(onToggleBP()));

  resetInterface();

  //statusBar()->addWidget(); //AddLabelsEtc

  ui->registerList->setModel(registers);
  ui->breakpointList->setModel(breakpoints);
  ui->codeList->setModel(code);

  connect(ui->actionLoad_program,SIGNAL(triggered()),this,SLOT(on_actionLoadProgram_triggered()));
  connect(ui->actionM68000,SIGNAL(triggered()),this,SLOT(new68000Processor()));
  connect(ui->actionM68360,SIGNAL(triggered()),this,SLOT(new68360Processor()));
  connect(this,SIGNAL(logOutputChanged()),this,SLOT(on_logOutput_changed()));
  connect(this,SIGNAL(newDeviceCreated()),this,SLOT(resetInterface()));
  connect(this,SIGNAL(registersChanged()),this,SLOT(onRegistersChanged()));
  connect(this,SIGNAL(newDeviceCreated()),code,SLOT(clear()));
  connect(code,SIGNAL(listingLoaded()),this,SLOT(onListingLoaded()));
  connect(this,SIGNAL(programCounterChanged(Address)),code,SLOT(onProgramCounterChanged(Address)));

  connect(this,SIGNAL(stepped()),this,SIGNAL(memoryChanged()));
  connect(this,SIGNAL(stepped()),this,SIGNAL(registersChanged()));
  connect(this,SIGNAL(stepped()),this,SLOT(onProgramCounterChanged()));

  connect(this,SIGNAL(startedRunning()),this,SLOT(onStartRunning()));
  connect(this,SIGNAL(stoppedRunning(QString)),this,SLOT(onStopRunning(QString)));

  QFont font("Monospace");
  font.setStyleHint(QFont::TypeWriter);
  ui->registerList->setFont(font);
  ui->breakpointList->setFont(font);
  ui->codeList->setFont(font);

  statusbarStatus.setText("Status: Stopped");
  ui->statusbar->addPermanentWidget(&statusbarStatus);
  statusbarExtraInfo.setText("");
  ui->statusbar->addWidget(&statusbarExtraInfo);

  ui->codeList->setUniformItemSizes(true);

  ui->actionLoad_program->setEnabled(false);

  connect(this,SIGNAL(newDeviceCreated()),memory,SLOT(memoryReset()));
}

MainWindow::~MainWindow()
{
  delete ui;
}

void MainWindow::on_consoleInput_returnPressed()
{
  if(running) {
    setStatusbarTemp(QString("Can't do that while running!"));
    return;
  }
  if(interface==NULL) {
    this->statusBar()->showMessage(QString("Process not started"),3000);
    return;
  }
  QString str = ui->consoleInput->text();
  ui->consoleInput->setText("");
  interface->ExecuteCommand(str.toUtf8().constData());
}

void MainWindow::on_actionLoadProgram_triggered()
{
  if(running) {
    setStatusbarTemp(QString("Can't do that while running!"));
    return;
  }
  if(interface==NULL) {
    setStatusbarTemp(QString("Processor not created yet!"));
    return;
  }
  QString fileName = QFileDialog::getOpenFileName(this,
      tr("Load program"), QDir::currentPath(), tr("Program (*.h68);;All files(*.*)"));
  if(fileName.isEmpty())
    return;
  setStatusbarTemp(QString("Loading " + fileName));
  if(!interface->LoadProgram(fileName.toUtf8().data(), 0)) {
    //this->statusBar()->showMessage("Failed to load  " + fileName, 3000);
    return;
  }
}

void MainWindow::on_actionLoad_setup_triggered()
{
  if(running) {
    setStatusbarTemp(QString("Can't do that while running!"));
    return;
  }
  QString fileName = QFileDialog::getOpenFileName(this,
      tr("Load setup"), QDir::currentPath(), tr("Setup (*.setup);;All files(*.*)"));
  if(fileName.isEmpty())
    return;
  this->statusBar()->showMessage("Loading " + fileName, 3000);
  QFile file(fileName);
  if(!file.open(QIODevice::ReadOnly)) {
    setStatusbarTemp(QString("Can't open %1!").arg(fileName));
  }

  QTextStream in(&file);
  while(!in.atEnd()) {
      QString line = in.readLine();
      if(line.startsWith("SIMULATOR")) {
        QString proc = line.replace(0,line.indexOf('{')+1, "");
        proc.chop(1);
        newProcessor(proc);
      } else if (line.startsWith("COMMAND")) {
        if(interface==NULL)
          break;
        QString cmd = line.replace(0,line.indexOf('{')+1, "");
        cmd.chop(1);
        interface->ExecuteCommand(cmd.toUtf8().data());
      }
  }

  file.close();
  interface->LogLn(fileName.toUtf8().constData());
  //interface->LoadProgram();
}

void MainWindow::on_logOutput_changed()
{
  QString str = QString(logBuffer.str().c_str());
  logBuffer.str(std::string());
  ui->consoleOutput->append(str);
}

QtInterface* MainWindow::newProcessor(QString type) {
  //std::unique_ptr<BasicCPU> processor;
  BasicCPU* processor;
  if(type == QString("M68000") || type == "sim68000")
    processor = new m68000();
    //processor = std::unique_ptr<BasicCPU>(new m68000);
  else if (type == QString("M68360") || type == "sim68360"){
    //  processor = std::unique_ptr<BasicCPU>(new cpu32);
    setStatusbarTemp(QString("Processor \"" + type + "\" not implemented yet!"));
    return NULL;
  } else {
    setStatusbarTemp(QString("Error creating processor \"" + type + "\""));
    return NULL;
  }
  //auto loader = std::unique_ptr<BasicLoader>(new Loader(*processor));
  //auto registry = std::unique_ptr<BasicDeviceRegistry>(new DeviceRegistry);
  BasicLoader* loader = new Loader(*processor);
  BasicDeviceRegistry* registry = new DeviceRegistry();
  //QtInterface newInterface(*processor, *registry, *loader, *this/*, logBuffer*/);
  //QtInterface* newInterface = new QtInterface(*processor, *registry, *loader, *this, logBuffer);
  QtInterface* newInterface = new QtInterface(*processor, *registry, *loader, *this, logBuffer);
  if(interface!=NULL) {
    disconnect(interface,SIGNAL(registerChanged(std::string,std::string)),registers,SLOT(onRegisterChanged(std::string,std::string)));
    disconnect(interface,SIGNAL(programLoaded(std::string)),code,SLOT(loadListing(std::string)));
    disconnect(interface,SIGNAL(stepped()),this,SIGNAL(stepped()));
    delete interface;
  }
  interface = newInterface;
  connect(interface,SIGNAL(registerChanged(std::string,std::string)),registers,SLOT(onRegisterChanged(std::string,std::string)));
  connect(interface,SIGNAL(programLoaded(std::string)),code,SLOT(loadListing(std::string)));
  connect(interface,SIGNAL(stepped()),this,SIGNAL(stepped()));
  setStatusbarTemp((std::string) "New M68000 process created");
  emit newDeviceCreated();
  ui->centralTabWidget->setCurrentIndex(1);
  emit registersChanged();
  ui->actionLoad_program->setEnabled(true);
  return newInterface;
}

void MainWindow::new68000Processor() {
  newProcessor(QString("M68000"));
}

void MainWindow::new68360Processor() {
  newProcessor(QString("M68360"));
}
void MainWindow::outputChanged() {
  emit logOutputChanged();
}

std::stringstream* MainWindow::GetLogBuffer() {
  return &logBuffer;
}

void MainWindow::resetInterface() {
  if(interface==NULL) {
    //ui->codeText->setText("");
    /*while (!ui->breakpointList)
        delete ui->breakpointList->takeItem(0);
    while (!ui->registerList)
        delete ui->registerList->takeItem(0);*/
    ui->traceText->setText("");
    ui->consoleDock->setEnabled(false);
    ui->traceDock->setEnabled(false);
    ui->breakpointDock->setEnabled(false);
    ui->registerDock->setEnabled(false);
    //ui->portDockA->setEnabled(false);
    //ui->portDockB->setEnabled(false);
    memoryDock->setEnabled(false);
    ui->deviceSettingsFrame->setEnabled(false);
    ui->centralTabWidget->setCurrentIndex(1);
    return;
  } else {
    //Call updates
    //update memory, breakpoints, registers
    ui->traceText->setText("");
    ui->consoleDock->setEnabled(true);
    ui->traceDock->setEnabled(true);
    ui->breakpointDock->setEnabled(true);
    ui->registerDock->setEnabled(true);
    //ui->portDockA->setEnabled(true);
    //ui->portDockB->setEnabled(true);
    memoryDock->setEnabled(true);
    ui->deviceSettingsFrame->setEnabled(true);
  }
}

void MainWindow::Trace(std::string str) {
  ui->traceText->append(QString::fromStdString(str));
}

void MainWindow::TraceLn(std::string str) {
  Trace(str + "\n");
}

void MainWindow::Log(std::string str) {
  ui->consoleOutput->append(QString::fromStdString(str));
}

void MainWindow::LogLn(std::string str) {
  Log(str + "\n");
}

void MainWindow::setStatusbarTemp(QString str) {
  ui->statusbar->showMessage(str,3000);
}

void MainWindow::setStatusbarTemp(std::string str) {
  ui->statusbar->showMessage(QString::fromStdString(str),3000);
}


void MainWindow::on_createProcessorButton_pressed()
{
  if(!running)
    newProcessor(ui->setupProcesssorComboBox->currentText());
  else
    setStatusbarTemp(QString("Can't do that while running!"));
}

void MainWindow::onRegistersChanged() {
  QList<RegisterModel>* registerList = new QList<RegisterModel>;
  interface->GetRegisters(*registerList);
  registers->update(*registerList);
  delete registerList;
}

void MainWindow::onStep() {
  if(running) {
    setStatusbarTemp(QString("Can't do that while running!"));
    return;
  }
  if(interface==NULL) {
    setStatusbarTemp(QString("Processor not loaded!"));
    return;
  }
  interface->Step(1);
}

void MainWindow::onRun() {
  if(running) {
    emit stopRunning();
  }
  if(interface==NULL) {
    setStatusbarTemp(QString("Processor not loaded!"));
    return;
  }
  interface->Run();
}

void MainWindow::onRunStep() {
  if(running) {
    emit stopRunning();
  }
  if(interface==NULL) {
    setStatusbarTemp(QString("Processor not loaded!"));
    return;
  }
  interface->RunStepping();
}

void MainWindow::onReset() {
  if(running) {
    setStatusbarTemp(QString("Can't do that while running!"));
    return;
  }
  if(interface==NULL) {
    setStatusbarTemp(QString("Processor not loaded!"));
    return;
  }
  interface->Reset();
  emit programCounterChanged(StringToInt(interface->ProgramCounterValue()));
  ui->codeList->scrollTo(code->getLastPCIndex(),QAbstractItemView::PositionAtCenter);
}

void MainWindow::onToggleBP() {
  if(running) {
    setStatusbarTemp(QString("Can't do that while running!"));
    return;
  }
  if(interface==NULL) {
    setStatusbarTemp(QString("Processor not loaded!"));
    return;
  }
  int i = ui->codeList->currentIndex().row();
  if(i<0) {
    setStatusbarTemp((std::string) "Select a line first!");
    return;
  }
  const CodeLine* line = code->lineAt(i);
  if(breakpoints->contains(line->getAddress()))
    interface->RemoveBreakpoint(line->getAddress());
  else
    interface->NewBreakpoint(line->getAddress());
}

void MainWindow::on_editRegisterButton_pressed()
{
  if(running) {
    setStatusbarTemp(QString("Can't do that while running!"));
    return;
  }
  int i = ui->registerList->currentIndex().row();
  if(i<0) {
    setStatusbarTemp(QString("Select a register first!"));
    return;
  }
  RegisterModel reg = registers->getByIndex(i);
  bool ok;
  QString text = QInputDialog::getText(this, tr("Set value of %1").arg(reg.getName()),
                                       tr("Value:"), QLineEdit::Normal,
                                       reg.getValue(), &ok).toUpper();
  if (ok && !text.isEmpty()) {
    std::string value = text.toUtf8().data();
    interface->SetRegister(reg.getName().toUtf8().data(),value);
    setStatusbarTemp(QString("Changed value of %1").arg(reg.getName()));
  }
}

void MainWindow::on_newMemViewButton_pressed()
{
  int index = ui->registerList->currentIndex().row();
  if(index < 0) {
    setStatusbarTemp(QString("Select a register first!"));
    return;
  }
  RegisterModel reg = registers->getByIndex(index);
  MemoryDockWidget* dock = new MemoryDockWidget(this, reg.getValue().toInt(NULL,16), true, this);
  //QMainWindow::tabifyDockWidget(memoryDock, dock);
  dock->setFloating(true);
  dock->adjustSize();
  dock->show();
  dock->raise();
}

void MainWindow::on_memViewButton_pressed()
{
  int index = ui->registerList->currentIndex().row();
  if(index < 0) {
    setStatusbarTemp(QString("Select a register first!"));
    return;
  }
  RegisterModel reg = registers->getByIndex(index);
  /*unsigned int address = StringToInt(reg.getValue().toUtf8().data());
  int spaces = interface->ListNumberOfAddressSpaces();
  int i;
  for(i=0;i<spaces;i++){
    std::cout <<interface->ListMaximumAddress(i)<<std::endl;
    if(address<interface->ListMaximumAddress(i))
      break;
  }
  if(i==spaces) {
    setStatusbarTemp(QString("Not a valid memory position!"));
    return;
  }*/
  memoryDock->scrollToAddress(reg.getValue().toInt(NULL,16));
}


void MainWindow::on_addBreakpointButton_pressed()
{
  if(running) {
    setStatusbarTemp(QString("Can't do that while running!"));
    return;
  }
  bool ok;
  QString text = QInputDialog::getText(this, tr("Create new breakpoint"),
                                       tr("Address:"), QLineEdit::Normal,
                                       "00000000", &ok);
  if (ok && !text.isEmpty()) {
    Address a = StringToInt(text.toUtf8().data());
    if(interface->NewBreakpoint(a))
      setStatusbarTemp(QString("Added breakpoint at address %1").arg(a, 8,16,QChar('0')));
  }
}

void MainWindow::on_removeBPButton_pressed()
{
  if(running) {
    setStatusbarTemp(QString("Can't do that while running!"));
    return;
  }
  int i = ui->breakpointList->currentIndex().row();
  if(i<0) {
    setStatusbarTemp(QString("Select a breakpoint first!"));
    return;
  }
  Address a = breakpoints->getByIndex(ui->breakpointList->currentIndex().row());
  if(interface->RemoveBreakpoint(a))
    setStatusbarTemp(QString("Removed breakpoint at address %1").arg(a, 8,16,QChar('0')).toUpper());
}

void MainWindow::onListingLoaded() {
  onProgramCounterChanged();
  ui->centralTabWidget->setCurrentIndex(0);
}

void MainWindow::onProgramCounterChanged() {
  if(interface==NULL)
    return;
  Address pc = StringToInt(interface->ProgramCounterValue());
  emit programCounterChanged(pc);
  ui->codeList->scrollTo(code->getLastPCIndex(),QAbstractItemView::PositionAtCenter);
}

void MainWindow::on_gotoBPButton_pressed()
{
  int index = ui->breakpointList->currentIndex().row();
  Address address = breakpoints->getByIndex(index);
  ui->codeList->scrollTo(code->getIndexFromAddress(address),QAbstractItemView::PositionAtCenter);
}

void MainWindow::onStopRunning(QString str) {
  running = false;
  LogLn(str.toUtf8().data());
  setStatusbarTemp(str);
  statusbarStatus.setText("Status: Stopped");

  stepAction->setEnabled(true);
  //runAction->setEnabled(true);
  //runStepAction->setEnabled(true);
  runAction->setText("Run");
  runStepAction->setText("RunStep");
  resetAction->setEnabled(true);
  toggleBPAction->setEnabled(true);
}

void MainWindow::onStartRunning() {
  running = true;
  statusbarStatus.setText("Status: Running");

  stepAction->setEnabled(false);
  //runAction->setEnabled(false);
  //runStepAction->setEnabled(false);
  runAction->setText("Stop");
  runStepAction->setText("Stop");
  resetAction->setEnabled(false);
  toggleBPAction->setEnabled(false);
}

bool MainWindow::isRunning() {
  return running;
}

void MainWindow::on_actionSet_RunStep_delay_triggered()
{
  bool ok;
  int value = QInputDialog::getInt(this, tr("Set delay"),
                            tr("Milliseconds:"),
                            runStepWait, 25, 1000, 5, &ok);
  if (ok)
    runStepWait = value;
}
