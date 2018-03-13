#ifndef DUARTPORTDOCKWIDGET_H
#define DUARTPORTDOCKWIDGET_H

#include <QDockWidget>
#include <QTextEdit>
#include <QMutex>
#include "Framework/Types.hpp"

class DUARTPortDockWidget;

class DUARTTextEdit : public QTextEdit
{
public:
  explicit DUARTTextEdit(DUARTPortDockWidget* dock, QWidget *parent);

private:
  DUARTPortDockWidget* dock;
  void keyPressEvent(QKeyEvent *e);
};

class DUARTPortDockWidget : public QDockWidget
{
  Q_OBJECT

public:
  explicit DUARTPortDockWidget(std::string title, QWidget *parent = 0);
  ~DUARTPortDockWidget();
  bool writeByte(Byte* c);
  bool readByte(Byte* c);
  bool insertByte(Byte* c);

private:
  DUARTTextEdit* console;
  QMutex writeMutex;
  QByteArray readBuffer = QByteArray();

public slots:
  void onCharWritten(QChar c);

signals:
  void charWritten(QChar c);

  friend class DUARTTextEdit;
};

#endif // DUARTPORTDOCKWIDGET_H
