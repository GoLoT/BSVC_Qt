#include "duartportdockwidget.h"
#include <QGridLayout>
#include <QByteArray>
#include <QApplication>
#include <QKeyEvent>
#include <QDebug>
#include "mainwindow.h"

DUARTPortDockWidget::DUARTPortDockWidget(std::string title, QWidget *parent) :
  QDockWidget(title.empty()?"Port":QString::fromStdString(title),parent)
{
  QFont font("Monospace");
  font.setStyleHint(QFont::TypeWriter);
  this->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
  this->setObjectName(QStringLiteral("portDock"));
  this->setStyleSheet(QLatin1String("QDockWidget::title {\n"
"    text-align: left;\n"
"    background: transparent;\n"
"    padding-left: 5px;\n"
"}\n"
"\n"
"QWidget {\n"
"    border: 1px solid lightgrey;\n"
"}"));
  QWidget* dockContent = new QWidget();
  QGridLayout* layout = new QGridLayout(dockContent);
  layout->setSpacing(6);
  layout->setContentsMargins(11, 11, 11, 11);
  console = new DUARTTextEdit(this, dockContent);
  console->setObjectName(QStringLiteral("portConsole"));
  console->setFrameShape(QFrame::StyledPanel);
  console->setFont(font);
  console->setStyleSheet("* { background-color: rgb(0, 0, 0); color: rgb(24, 175, 72); }");
  layout->addWidget(console, 0, 0, 1, 1);
  this->setWidget(dockContent);
  /*MainWindow* mainWin  = NULL;
  foreach (QWidget *w, QApplication::topLevelWidgets()) {
    if (mainWin = qobject_cast<MainWindow*>(w)) {
      this->setParent(w);
      break;
    }
  }*/
  this->show();
  connect(this,SIGNAL(charWritten(QChar)),this,SLOT(onCharWritten(QChar)));
  if(parent)
    qobject_cast<MainWindow*>(parent)->addDockWidget(Qt::BottomDockWidgetArea, this);
  else
    this->setFloating(true);
}

DUARTPortDockWidget::~DUARTPortDockWidget()
{
  disconnect(this,SIGNAL(charWritten(QChar)),this,SLOT(onCharWritten(QChar)));
  delete console;
}

bool DUARTPortDockWidget::writeByte(Byte* c) {
  emit charWritten(QChar(*c));
  return true;
}

bool DUARTPortDockWidget::readByte(Byte* c) {
  bool ret = false;
  if(!readBuffer.isEmpty()) {
    writeMutex.lock();
    if(!readBuffer.isEmpty()) {
      *c = readBuffer.at(0);
      readBuffer.remove(0,1);
      ret = true;
    }
    writeMutex.unlock();
    return ret;
  }
  return ret;
}

bool DUARTPortDockWidget::insertByte(Byte* c) {
  writeMutex.lock();
  readBuffer.append((char) *c);
  writeMutex.unlock();
  return true;
}

void DUARTPortDockWidget::onCharWritten(QChar c) {
  //console->append(c);
  if(c==10)
    return;
  console->moveCursor (QTextCursor::End);
  console->insertPlainText (c);
  console->moveCursor (QTextCursor::End);
}

DUARTTextEdit::DUARTTextEdit(DUARTPortDockWidget* dock, QWidget* parent) : QTextEdit(parent) {
  this->dock = dock;
}

void DUARTTextEdit::keyPressEvent(QKeyEvent *e) {
  Byte c;
  int key = e->key();
  switch (e->key()) {
    case Qt::Key_Escape:
      break;
    case Qt::Key_Space:
      c = key;
      dock->insertByte(&c);
      break;
    case Qt::Key_Enter:
    case Qt::Key_Return:
      c = 13;
      dock->insertByte(&c);
      break;
    default:
      if(e->key() >= Qt::Key_A && e->key() <= Qt::Key_Z) {
        if(e->modifiers() & Qt::ShiftModifier)
          c = key;
        else
          c = key+32;
        dock->insertByte(&c);
      }
  }
  //QTextEdit::keyPressEvent(e);
  //qDebug() << e->key();
}
