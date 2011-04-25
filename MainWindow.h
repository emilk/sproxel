#ifndef __MAIN_WINDOW_H__
#define __MAIN_WINDOW_H__

#include <QWidget>

class QSlider;
class GLModelWidget;

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    MainWindow();

protected:
    void keyPressEvent(QKeyEvent* event);
    //void keyReleaseEvent(QKeyEvent *event);
    
private:
    GLModelWidget* m_glModelWidget;
    
    //bool eventFilter(QObject* qo, QEvent* ev);
};

#endif
