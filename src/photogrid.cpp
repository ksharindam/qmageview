/* This file is a part of qmageview program, which is GPLv3 licensed */

#include <QDebug>
#include "common.h"
#include "photogrid.h"
#include "pdfwriter.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QPainter>
#include <QPen>
#include <QDesktopWidget>
#include <QSettings>
#include <QBuffer>
#include <cmath>


GridDialog:: GridDialog(QImage img, QWidget *parent) : QDialog(parent)
{
    setupUi(this);
    resize(1020, 640);
    QHBoxLayout *layout = new QHBoxLayout(scrollAreaWidgetContents);
    layout->setContentsMargins(0, 0, 0, 0);
    gridPaper = new GridPaper(this);
    layout->addWidget(gridPaper);
    thumbnailGr = new ThumbnailGroup(this);
    Thumbnail *thumbnail = new Thumbnail(img, frame);
    verticalLayout->addWidget(thumbnail);
    thumbnail->select(true);
    QObject::connect(thumbnail, SIGNAL(clicked(QImage)), gridPaper, SLOT(setPhoto(QImage)));
    thumbnailGr->append(thumbnail);
    QObject::connect(configureBtn, SIGNAL(clicked()), this, SLOT(configure()));
    QObject::connect(addPhotoBtn, SIGNAL(clicked()), this, SLOT(addPhoto()));
    QObject::connect(checkAddBorder, SIGNAL(clicked(bool)), gridPaper, SLOT(toggleBorder(bool)));
    QObject::connect(helpBtn, SIGNAL(clicked()), this, SLOT(showHelp()));
    gridPaper->photo = img;
}

void
GridDialog:: configure()
{
    GridSetupDialog *dialog = new GridSetupDialog(this);
    if ( dialog->exec()==QDialog::Accepted ) {
        gridPaper->paperW = dialog->paperW;
        gridPaper->paperH = dialog->paperH;
        gridPaper->rows = dialog->rows;
        gridPaper->cols = dialog->cols;
        gridPaper->W = dialog->W;
        gridPaper->H = dialog->H;
        gridPaper->DPI = dialog->DPI;
        gridPaper->setupGrid();
    }
}

void
GridDialog:: addPhoto()
{
    QString filefilter = "JPEG Images (*.jpg *jpeg);;PNG Images (*.png);;All Files (*)";
    QString filepath = QFileDialog::getOpenFileName(this, "Open Image", "", filefilter);
    if (filepath.isEmpty()) return;
    QImage img = loadImage(filepath);
    if (not img.isNull()) {
        Thumbnail *thumbnail = new Thumbnail(img, frame);
        verticalLayout->addWidget(thumbnail);
        QObject::connect(thumbnail, SIGNAL(clicked(QImage)), gridPaper, SLOT(setPhoto(QImage)));
        thumbnailGr->append(thumbnail);
    }
}

void
GridDialog:: accept()
{
    // Create final grid when ok is clicked
    gridPaper->createFinalGrid();
    QDialog::accept();
}

void
GridDialog:: showHelp()
{
    QString helptext = "Click on a image thumbnail to select an image to drop. Then click on the blank boxes to drop the selected photo.\n\n"
                       "If you want to create grid with more different photos then load photo by clicking Add Photo button.\n\n"
                       "You can change the photo of a box by selecting another image and clicking over the box.";
    QMessageBox::about(this, "How to Create Grid", helptext);
}


Thumbnail:: Thumbnail(QImage img, QWidget *parent) : QLabel(parent)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    photo = img;
    setPixmap(QPixmap::fromImage(img.scaledToWidth(100)));
}

void
Thumbnail:: mousePressEvent(QMouseEvent *)
{
    emit clicked(photo);
}

void
Thumbnail:: select(bool selected)
{
    if (selected) {
        QImage img = photo.scaledToWidth(100);
        QPainter painter(&img);
        QPen pen(Qt::blue);
        pen.setWidth(4);
        painter.setPen(pen);
        painter.drawRect(2, 2 , 100-4, img.height()-4);
        painter.end();
        setPixmap(QPixmap::fromImage(img));
    }
    else
        setPixmap(QPixmap::fromImage(photo.scaledToWidth(100)));
}


ThumbnailGroup:: ThumbnailGroup(QObject *parent) : QObject(parent)
{
}

void
ThumbnailGroup:: append(Thumbnail *thumbnail)
{
    thumbnails << thumbnail;
    QObject::connect(thumbnail, SIGNAL(clicked(QImage)), this, SLOT(selectThumbnail(QImage)));
}

void
ThumbnailGroup:: selectThumbnail(QImage)
{
    for (int i=0;i<thumbnails.count();++i) {
        thumbnails[i]->select(false);
    }
    qobject_cast<Thumbnail*>(sender())->select(true);
}

// GridPaper class methods
GridPaper:: GridPaper(QWidget *parent) : QLabel(parent)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setMouseTracking(true);
    add_border = true;
    QSettings settings(this);
    DPI = settings.value("DPI", 300).toInt();
    paperW = settings.value("PaperWidth", 1800).toInt();
    paperH = settings.value("PaperHeight", 1200).toInt();
    W = settings.value("ImageWidth", 413).toInt();
    H = settings.value("ImageHeight", 531).toInt();
    cols = settings.value("Cols", 4).toInt();
    rows = settings.value("Rows", 2).toInt();           // total no. of columns and rows
    setupGrid();
}

void
GridPaper:: setupGrid()
{
    boxes.clear();
    spacingX = (paperW-cols*W)/float(cols+1);
    spacingY = (paperH-rows*H)/float(rows+1);
    // Setup Foreground Grid
    float screenDPI = QApplication::desktop()->logicalDpiX();
    scale = screenDPI/DPI;
    float w = W*scale;
    float h = H*scale;
    float spacing_x = spacingX*scale;
    float spacing_y = spacingY*scale;
    for (int i=0; i<cols*rows; ++i) {
        int row = i/cols;            // Position of the box as row & col
        int col = i%cols;
        QRect box = QRect(spacing_x+col*(spacing_x+w), spacing_y+row*(spacing_y+h), w-1, h-1);
        boxes << box;
    }
    QPixmap pm = QPixmap(paperW*scale, paperH*scale);
    pm.fill();
    QPainter painter(&pm);
    foreach (QRect box, boxes)
        painter.drawRect(box);
    painter.end();
    setPixmap(pm);
}

void
GridPaper:: setPhoto(QImage img)
{
    photo = img;
}

void
GridPaper:: toggleBorder(bool ok)
{
    add_border = ok;
    QPixmap grid = *(pixmap());
    QPainter painter(&grid);
    foreach (int index, image_dict.keys()) {
        QPoint topleft = boxes[index].topLeft();
        QImage img = image_dict.value(index).scaled(W*scale, H*scale, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        painter.drawImage(topleft, img);
        if (ok) painter.drawRect(topleft.x(), topleft.y(), img.width()-1, img.height()-1);
    }
    painter.end();
    setPixmap(grid);
}

void
GridPaper:: mouseMoveEvent(QMouseEvent *ev)
{
    // Change cursor whenever cursor comes over a box
    foreach (QRect box, boxes) {
        if (box.contains(ev->pos())) {
            setCursor(Qt::PointingHandCursor);
            return;
        }
    }
    setCursor(Qt::ArrowCursor);
}

void
GridPaper:: mousePressEvent(QMouseEvent *ev)
{
    foreach (QRect box, boxes) {
        if (box.contains(ev->pos())) {
            QPoint topleft = box.topLeft();
            QImage img = photo.scaled(W*scale, H*scale, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            QPixmap bg = *(pixmap());
            QPainter painter(&bg);
            painter.fillRect(topleft.x(), topleft.y(), W*scale, H*scale, Qt::white);
            painter.drawImage(topleft, img);
            if (add_border)
                painter.drawRect(topleft.x(), topleft.y(), img.width()-1, img.height()-1);
            painter.end();
            setPixmap(bg);
            image_dict[boxes.indexOf(box)] = photo;
            break;
        }
    }
}

void
GridPaper:: createFinalGrid()
{
    photo_grid = QImage(paperW, paperH, QImage::Format_ARGB32);
    photo_grid.fill(Qt::white);
    QPainter painter(&photo_grid);
    foreach (int index, image_dict.keys()) {
        int row = index/cols;
        int col = index%cols;
        QPoint topleft = QPoint(spacingX+col*(spacingX+W), spacingY+row*(spacingY+H));
        QImage img = image_dict.value(index).scaled(W, H, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        painter.drawImage(topleft, img);
        if (add_border)
            painter.drawRect(topleft.x(), topleft.y(), img.width()-1, img.height()-1);
    }
    painter.end();
}

// GridSetupDialog class functions
GridSetupDialog:: GridSetupDialog(QWidget *parent) : QDialog(parent)
{
    setupUi(this);
}

void
GridSetupDialog:: accept()
{
    QList<float> units;
    units << 1 << 1/2.54 << 1/25.4 ;
    DPI = spinDPI->value();
    float unit_mult = units[paperSizeUnit->currentIndex()];
    int paper_w = spinPaperWidth->value()*unit_mult*DPI;
    int paper_h = spinPaperHeight->value()*unit_mult*DPI;
    W = spinPhotoWidth->value()*DPI/2.54;
    H = spinPhotoHeight->value()*DPI/2.54;
    int rows1 = paper_h/H;
    int cols1 = paper_w/W;
    int rows2 = paper_w/H;
    int cols2 = paper_h/W;
    if (rows1*cols1 >= rows2*cols2) {
        paperW = paper_w;
        paperH = paper_h;
        rows = rows1;
        cols = cols1;
    }
    else {
        paperW = paper_h;
        paperH = paper_w;
        rows = rows2;
        cols = cols2;
    }
    QSettings settings(this);
    settings.setValue("DPI", DPI);
    settings.setValue("PaperWidth", paperW);
    settings.setValue("PaperHeight", paperH);
    settings.setValue("ImageWidth", W);
    settings.setValue("ImageHeight", H);
    settings.setValue("Rows", rows);
    settings.setValue("Cols", cols);
    QDialog::accept();
}


// *************************** ------------- ***************************
// *************************** Photo Collage ***************************
// *************************** ------------- ***************************

// ******************* Collage Dialog ***********************

CollageDialog:: CollageDialog(QWidget *parent) : QDialog(parent)
{
    setupUi(this);
    resize(1050, 716);
    QHBoxLayout *layout = new QHBoxLayout(scrollAreaWidgetContents);
    layout->setContentsMargins(0, 0, 0, 0);
    QSettings settings;
    int W = settings.value("CollageW", 1024).toInt();
    int H = settings.value("CollageH", 720).toInt();
    int pdf_w = settings.value("CollagePdfW", 595).toInt();
    int pdf_h = settings.value("CollagePdfH", 842).toInt();
    int dpi = settings.value("CollagePdfDpi", 300).toInt();
    collagePaper = new CollagePaper(this, W, H, pdf_w, pdf_h, dpi);
    layout->addWidget(collagePaper);
    connect(collagePaper, SIGNAL(statusChanged(QString)), this, SLOT(showStatus(QString)));
    connect(addBtn, SIGNAL(clicked()), collagePaper, SLOT(addPhoto()));
    connect(removeBtn, SIGNAL(clicked()), collagePaper, SLOT(removePhoto()));
    connect(rotateBtn, SIGNAL(clicked()), collagePaper, SLOT(rotatePhoto()));
    connect(addBorderBtn, SIGNAL(clicked()), collagePaper, SLOT(toggleBorder()));
    connect(configBtn, SIGNAL(clicked()), this, SLOT(setupBackground()));
    connect(savePdfBtn, SIGNAL(clicked()), collagePaper, SLOT(savePdf()));
    connect(okBtn, SIGNAL(clicked()), this, SLOT(accept()));
    connect(cancelBtn, SIGNAL(clicked()), this, SLOT(reject()));
    savePdfBtn->setEnabled(dpi != 0);
}

void
CollageDialog:: setupBackground()
{
    CollageSetupDialog *dlg = new CollageSetupDialog(this);
    if (dlg->exec() == QDialog::Rejected) return;
    QSettings settings;
    collagePaper->background_filename = "";
    collagePaper->dpi = 0;
    if (dlg->resolutionBtn->isChecked()) {
        collagePaper->W = dlg->widthEdit->text().toInt();
        collagePaper->H = dlg->heightEdit->text().toInt();
        settings.setValue("CollageW", collagePaper->W);
        settings.setValue("CollageH", collagePaper->H);
        settings.setValue("CollagePdfDpi", 0);  // this prevents using pdf paper size
    }
    else if (dlg->pageSizeBtn->isChecked()) {
        // convert other units to point (1/72 inch))
        float factor = 1.0;
        if (dlg->unitsCombo->currentText() == QString("in")) {
            factor = 72.0;
        }
        else if (dlg->unitsCombo->currentText() == QString("mm"))
            factor = 72/25.4;
        collagePaper->pdf_w = dlg->pageWidth->value()*factor;
        collagePaper->pdf_h = dlg->pageHeight->value()*factor;
        collagePaper->dpi = dlg->dpiCombo->currentText().toInt();
        settings.setValue("CollagePdfW", collagePaper->pdf_w);
        settings.setValue("CollagePdfH", collagePaper->pdf_h);
        settings.setValue("CollagePdfDpi", collagePaper->dpi);
    }
    else {
        collagePaper->background_filename = dlg->filename;
    }
    savePdfBtn->setEnabled(collagePaper->dpi != 0);
    collagePaper->setup();
}

void
CollageDialog:: showStatus(QString status)
{
    statusbar->setText(status);
}

void
CollageDialog:: accept()
{
    collage = collagePaper->getCollage();
    collagePaper->clean();
    QDialog::accept();
}

void
CollageDialog:: reject()
{
    collagePaper->clean();
    QDialog::reject();
}

// *************************** Collage Paper ***************************

// get a size of collage paper that fits inside window, and keeps aspect ratio
void getOptimumSize(int W, int H, int &out_w, int &out_h)
{
    if (W > H) { // landscape
        fitToSize(W,H, 1000,670, out_w, out_h);
    }
    else {
        if (W<=800) {
            out_w = W;
            out_h = H;
            return;
        }
        out_w = 800;
        out_h = round((800.0/W)*H);
    }
}

CollagePaper:: CollagePaper(QWidget *parent, int w, int h, int pdf_w, int pdf_h, int dpi)
                        : QLabel(parent), W(w), H(h), dpi(dpi), pdf_w(pdf_w), pdf_h(pdf_h)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setMouseTracking(true);
    background_filename = "";
    drag_icon = QPixmap(":/images/drag.png");
    setup();
}

void
CollagePaper:: setup()
{
    // load and set bg image
    clean();
    if (dpi) {
        W = pdf_w*dpi/72.0;
        H = pdf_h*dpi/72.0;
        // calculate optimum paper resolution (assuming 100 dpi screen)
        paper = QPixmap(round(pdf_w*100/72.0), round(pdf_h*100/72.0));
        paper.fill();
    }
    else if (not background_filename.isEmpty()) {
        QImage img = loadImage(background_filename);
        W = img.width();
        H = img.height();
        int opt_w, opt_h;
        getOptimumSize(W,H, opt_w, opt_h);
        paper = QPixmap::fromImage(img.scaled(opt_w, opt_h));
    }
    else {
        int opt_w, opt_h;
        getOptimumSize(W,H, opt_w, opt_h);
        paper = QPixmap(opt_w, opt_h);
        paper.fill();
    }
    setPixmap(paper);
}

void
CollagePaper:: addPhoto()
{
    QString filefilter = "Image files (*.jpg *.jpeg)";
    QStringList filenames = QFileDialog::getOpenFileNames(this, "Add Photos", "", filefilter);
    if (filenames.isEmpty()) return;
    for (QString filepath : filenames) {
        CollageItem *item = new CollageItem(filepath);
        item->w = round(item->img_w*100/300.0); // 300 dpi image over 100 ppi screen
        item->h = round(item->img_h*100/300.0);
        if (item->w > paper.width() or item->h > paper.height())
            fitToSize(item->img_w, item->img_h, paper.width(), paper.height(), item->w, item->h);
        collageItems.append(item);
    }
    draw();
    updateStatus();
}

void
CollagePaper:: removePhoto()
{
    if (collageItems.isEmpty()) return;
    CollageItem *item = collageItems.takeLast();
    delete item;
    draw();
    updateStatus();
}

void
CollagePaper:: rotatePhoto()
{
    if (collageItems.isEmpty()) return;
    CollageItem *item = collageItems.last();
    item->rotation += 90;
    item->rotation %= 360;  // keep angle between 0 and 359
    int tmp = item->w;
    item->w = item->h;
    item->h = tmp;
    QTransform tfm;
    tfm.rotate(90);
    item->pixmap = item->pixmap.transformed(tfm);
    draw();
}

void
CollagePaper:: toggleBorder()
{
    if (collageItems.isEmpty()) return;
    CollageItem *item = collageItems.last();
    item->border = not item->border;
    draw();
}

void
CollagePaper:: mousePressEvent(QMouseEvent *ev)
{
    clk_pos = ev->pos();
    for (int i=collageItems.size()-1; i>=0; i--)
    {
        CollageItem *item = collageItems.at(i);
        if (item->contains(ev->pos()))
        {
            collageItems.removeAt(i);
            collageItems.append(item);
            mouse_pressed = true;
            if (item->overCorner(ev->pos()))
                corner_clicked = true;
            break;
        }
    }
    draw();
    updateStatus();
}

void
CollagePaper:: mouseMoveEvent(QMouseEvent *ev)
{
    if (not mouse_pressed) return;
    if (collageItems.isEmpty()) return;
    CollageItem *item = collageItems.last();
    QPoint moved = ev->pos() - clk_pos;
    clk_pos = ev->pos();
    if (corner_clicked) {
        int box_w = item->w + moved.x();
        int box_h = item->h + moved.y();
        double img_aspect = item->pixmap.width()/(double)item->pixmap.height();
        int w = round(box_h * img_aspect);
        int h = round(box_w / img_aspect);
        if (h<=box_h) {
            item->w = box_w;
            item->h = h;
        }
        else {
            item->h = box_h;
            item->w = w;
        }
    }
    else
    {
        int new_x = MAX(item->x+moved.x(), 0);
        int new_y = MAX(item->y+moved.y(), 0);
        item->x = MIN(new_x, paper.width()-item->w);
        item->y = MIN(new_y, paper.height()-item->h);
    }
    draw();
    updateStatus();
}

void
CollagePaper:: mouseReleaseEvent(QMouseEvent */*ev*/)
{
    mouse_pressed = false;
    corner_clicked = false;
}

void
CollagePaper:: draw()
{
    QPixmap pm = paper.copy();
    QPainter painter(&pm);
    if (dpi) {              // my printer has bottom margin of 41.05 pt
        painter.setPen(Qt::gray);
        int offset = round((42.0*pm.height())/pdf_h);
        painter.drawLine(offset, pm.height()-offset, pm.width()-offset, pm.height()-offset);
    }
    for (CollageItem *item : collageItems)
    {
        painter.drawPixmap(item->x, item->y, item->w, item->h, item->pixmap);
        if (item == collageItems.last()) {     // highlight the selected (last) item
            QPen pen(Qt::blue, 2);
            painter.setPen(pen);
            painter.drawRect(item->x+1, item->y+1, item->w-2, item->h-2);
            painter.setPen(Qt::black);
        }
        if (item->border)
            painter.drawRect(item->x, item->y, item->w-1, item->h-1);
        painter.drawPixmap(item->x+item->w-16, item->y+item->h-16, drag_icon);
    }
    painter.end();
    setPixmap(pm);
}

QImage
CollagePaper:: getCollage()
{
    double scaleX = ((double)W)/paper.width();
    double scaleY = ((double)H)/paper.height();
    QPixmap pm;
    if (not background_filename.isEmpty())
        pm = QPixmap::fromImage(loadImage(background_filename));
    else {
        pm = QPixmap(W, H);
        pm.fill();
    }
    QPainter painter(&pm);
    for (CollageItem *item : collageItems)
    {
        QPixmap pixmap = QPixmap::fromImage(loadImage(QString::fromStdString(item->filename)));
        if (item->rotation) {
            QTransform tfm;
            tfm.rotate(item->rotation);
            pixmap = pixmap.transformed(tfm);
        }
        pixmap = pixmap.scaled(round(item->w*scaleX), round(item->h*scaleY),
                            Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        painter.drawPixmap(round(item->x*scaleX), round(item->y*scaleY), pixmap);
        if (item->border)
            painter.drawRect(round(item->x*scaleX), round(item->y*scaleY),
                            pixmap.width()-1, pixmap.height()-1);
    }
    return pm.toImage();
}

void
CollagePaper:: savePdf()
{
    if (collageItems.isEmpty()) return;
    QFileInfo fi(QString::fromStdString(collageItems.last()->filename));
    QString dir = fi.dir().path();
    QString filename("photo-collage.pdf");
    QString path(dir+ "/" + filename);
    int num=1;
    while (QFileInfo(path).exists()) {
        filename = "photo-collage-" + QString::number(num++) + ".pdf";
        path = dir + "/" + filename;
    }
    float scaleX = pdf_w/(float)paper.width();
    float scaleY = pdf_h/(float)paper.height();
    PdfWriter writer;
    writer.begin(path.toStdString());
    PdfObj cont;
    std::string cont_strm = "";
    PdfDict resources;
    PdfDict imgs;
    PdfObj img;
    img.set("Type", "/XObject");
    img.set("Subtype", "/Image");
    img.set("ColorSpace", "/DeviceRGB");
    img.set("BitsPerComponent", "8");
    img.set("Filter", "/DCTDecode"); // jpg = DCTDecode, for png = FlateDecode
    for (int i=0; i<collageItems.count(); i++)
    {
        CollageItem *item = collageItems.at(i);
        img.set("Width", item->img_w);
        img.set("Height", item->img_h);
        if (item->border) {
            QByteArray bArray;
            QBuffer buffer(&bArray);
            buffer.open(QIODevice::WriteOnly);
            QImage image = loadImage(QString::fromStdString(item->filename));
            QPainter painter(&image);
            painter.drawRect(0,0, image.width()-1, image.height()-1);
            painter.end();
            image.save(&buffer, "JPG");
            std::string data(bArray.data(), bArray.size());
            writer.addObj(img, data);
            bArray.clear();
            buffer.close();
        }
        else
            writer.addObj(img, readFile(item->filename));
        std::string matrix = imgMatrix(item->x*scaleX,
                                pdf_h - item->y*scaleY - item->h*scaleY, // img Y to pdf Y
                                item->w*scaleX, item->h*scaleY, item->rotation);
        cont_strm += format("q %s /img%d Do Q\n", matrix.c_str(), i);
        imgs.set(format("img%d",i), img.byref());
    }
    writer.addObj(cont, cont_strm);
    resources.set("XObject", imgs);
    PdfObj page = writer.createPage(pdf_w, pdf_h);
    page.set("Contents", cont);
    page.set("Resources", resources);
    writer.addPage(page);
    writer.finish();
    Notifier *notifier = new Notifier(this);
    notifier->notify("PDF Saved !", "Pdf Saved as \n" + filename);
}

void
CollagePaper:: updateStatus()
{
    if (collageItems.isEmpty()) {
        emit statusChanged(QString(""));
        return;
    }
    CollageItem *item = collageItems.last();
    QString status;
    int w = item->w*((float)W)/paper.width();
    int h = item->h*((float)H)/paper.height();
    status.sprintf("Resolution : %dx%d", w, h);
    if (dpi) {
        float w_cm = item->w*pdf_w/(float)paper.width()*2.54/72;
        float h_cm = item->h*pdf_h/(float)paper.height()*2.54/72;
        status += QString().sprintf(",   Size : %.1fx%.1f cm", w_cm, h_cm);
    }
    emit statusChanged(status);
}

void
CollagePaper:: clean()
{
    while (not collageItems.isEmpty()) {
        CollageItem *item = collageItems.takeLast();
        delete item;
    }
}

// ******************* Collage Item *********************

CollageItem:: CollageItem(QString filename) : x(0), y(0)
{
    // resize it
    pixmap = QPixmap::fromImage(loadImage(filename));
    img_w = pixmap.width();
    img_h = pixmap.height();
    if (img_w > 600 and img_h > 600)
        pixmap = pixmap.scaled(600, 600, Qt::KeepAspectRatio,Qt::SmoothTransformation);
    this->filename = filename.toStdString();
}

bool
CollageItem:: contains(QPoint pos)
{
    return ((x <= pos.x() and pos.x() < x+w) and
            (y <= pos.y() and pos.y() < y+h));
}

bool
CollageItem:: overCorner(QPoint pos)
{
    return (x+w-20 <= pos.x() and pos.x() < x+w and
            y+h-20 <= pos.y() and pos.y() < y+h);
}

// ****************** Collage Setup Dialog ****************** //

CollageSetupDialog:: CollageSetupDialog(QWidget *parent) : QDialog(parent)
{
    setupUi(this);
    connect(selectFileBtn, SIGNAL(clicked()), this, SLOT(selectFile()));
    connect(pageSizeCombo, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(toggleUsePageSize(const QString&)));
    filename = "";
}

void
CollageSetupDialog:: selectFile()
{
    if (not bgPhotoBtn->isChecked()) return;
    QString filefilter = "Image files (*.jpg *.png *.jpeg);;JPEG Images (*.jpg *.jpeg);;"
                         "PNG Images (*.png)";
    QString filepath = QFileDialog::getOpenFileName(this, "Add Photo", "", filefilter);
    if (filepath.isEmpty()) return;
    this->filename = filepath;
    QString text = QFileInfo(filepath).fileName();
    QFontMetrics fontMetrics(filenameLabel->font());
    text = fontMetrics.elidedText(text, Qt::ElideMiddle, filenameLabel->width()-2);
    filenameLabel->setText(text);
}

void
CollageSetupDialog:: toggleUsePageSize(const QString& text)
{
    widget_2->setEnabled(text == QString("Custom"));
}

void
CollageSetupDialog:: accept()
{
    if (resolutionBtn->isChecked()) {
        if (widthEdit->text().isEmpty() or heightEdit->text().isEmpty()) return;
    }
    else if (pageSizeBtn->isChecked()) {
        QString text = pageSizeCombo->currentText();
        if (text == QString("A4")) {
            pageWidth->setValue(595.0);
            pageHeight->setValue(842.0);
        }
        else if (text == QString("A5")) {
            pageWidth->setValue(420.0);
            pageHeight->setValue(595.0);
        }
        else if (text == QString("4x6 inch")) {
            pageWidth->setValue(432.0);
            pageHeight->setValue(288.0);
        }
        if (text != QString("Custom"))
            unitsCombo->setCurrentIndex(0); // unit = pt
    }
    else if (bgPhotoBtn->isChecked()) {
        if (filename.isEmpty()) return;
    }
    QDialog::accept();
}
