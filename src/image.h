/*
This file is a part of qmageview program, which is GPLv3 licensed

Image Label Object to display the image.
*/
#ifndef IMAGE_H
#define IMAGE_H

#include <QLabel>
#include <QWidget>
#include <QMovie>
#include <QPixmap>
#include <QPoint>
#include <QMouseEvent>
#include <QScrollArea>
#include <QScrollBar>

QT_BEGIN_NAMESPACE

//This is the image widget responsible for displaying image
class Image : public QLabel
{
    Q_OBJECT
public:
    Image(QWidget *parent, QScrollArea *scrollArea);
    void setAnimation(QMovie *anim);
    void setImage(QPixmap pixmap);
    void showScaled();
    void rotate(int degree);
    void zoomBy(float factor);
    void enableCropMode(bool enable);
    // Variables
    bool crop_mode, animation;
    float scale, scaleW, scaleH;
    QPixmap pic;
    QPoint p1, p2;
private:
    void mousePressEvent(QMouseEvent *ev);
    void mouseReleaseEvent(QMouseEvent *ev);
    void mouseMoveEvent(QMouseEvent *ev);
    void drawCropBox();
    // Variables
    int clk_area;
    bool mouse_pressed, lock_crop_ratio;
    float crop_width, crop_height, imgAspect;
    QPixmap pm_tmp;
    QPoint topleft, btmright, last_pt, clk_pos;
    // Click and drag handling variables
    int v_scrollbar_pos, h_scrollbar_pos;
    QPoint clk_global;
    QScrollBar *vScrollbar, *hScrollbar;
private slots:
    void lockCropRatio(bool checked);
    void setCropWidth(double value);
    void setCropHeight(double value);
    void cropImage();
signals:
    void imageUpdated();
};

QT_END_NAMESPACE
#endif //IMAGE_H