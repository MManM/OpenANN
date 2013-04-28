#include <OpenANN/RBM.h>
#include <OpenANN/io/Logger.h>
#include <OpenANN/io/DirectStorageDataSet.h>
#include <OpenANN/optimization/MBSGD.h>
#include <OpenANN/optimization/StoppingCriteria.h>
#include "IDXLoader.h"
#include <QGLWidget>
#include <QKeyEvent>
#include <QApplication>
#include <GL/glu.h>
#ifdef PARALLEL_CORES
#include <omp.h>
#endif

class RBMVisualization : public QGLWidget
{
  OpenANN::RBM& rbm;
  OpenANN::DataSet& dataSet;
  int visualizedNeurons;
  int rows, cols;
  int width, height;
  int instance;
  int offset;
  int fantasy;
public:
  RBMVisualization(OpenANN::RBM& rbm, OpenANN::DataSet& dataSet,
                   int visualizedNeurons, int rows, int cols,
                   QWidget* parent = 0, const QGLWidget* shareWidget = 0,
                   Qt::WindowFlags f = 0)
      : rbm(rbm), dataSet(dataSet), visualizedNeurons(visualizedNeurons),
        rows(rows), cols(cols), width(800), height(600), instance(0),
        offset(0), fantasy(0)
  {
  }

  virtual void initializeGL()
  {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthFunc(GL_LEQUAL);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glShadeModel(GL_SMOOTH);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glPointSize(5.0);
  }

  virtual void resizeGL(int width, int height)
  {
    this->width = width;
    this->height = height;
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, (float) width / (float) height, 1.0f, 200.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClearDepth(1.0f);
  }

  virtual void paintGL()
  {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    int xOffset = -70;
    int yOffset = -50;
    int zoom = -200;
    glTranslatef(xOffset, yOffset, zoom);

    Eigen::VectorXd x = dataSet.getInstance(instance);

    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(5.0);

    float scale = 1.0;

    glBegin(GL_QUADS);
    for(int xIdx = 0; xIdx < 28; xIdx++)
    {
      for(int yIdx = 0; yIdx < 28; yIdx++)
      {
        float c = x(yIdx*28+xIdx);
        float x = xIdx*scale;
        float y = 28.0f*scale - yIdx*scale;
        glColor3f(c, c, c);
        glVertex2f(x, y);
        glVertex2f(x+scale, y);
        glVertex2f(x+scale, y+scale);
        glVertex2f(x, y+scale);
      }
    }
    glEnd();

    Eigen::VectorXd r = rbm.reconstruct(instance, 1);
    glBegin(GL_QUADS);
    for(int xIdx = 0; xIdx < 28; xIdx++)
    {
      for(int yIdx = 0; yIdx < 28; yIdx++)
      {
        float c = r(yIdx*28+xIdx);
        float x = 30.0f + xIdx*scale;
        float y = 28.0f*scale - yIdx*scale;
        glColor3f(c, c, c);
        glVertex2f(x, y);
        glVertex2f(x+scale, y);
        glVertex2f(x+scale, y+scale);
        glVertex2f(x, y+scale);
      }
    }
    glEnd();

    for(int i = 0; i < 3; i++)
    {
      Eigen::VectorXd rp = rbm.reconstructProb(instance, 1+i+fantasy);
      glBegin(GL_QUADS);
      for(int xIdx = 0; xIdx < 28; xIdx++)
      {
        for(int yIdx = 0; yIdx < 28; yIdx++)
        {
          float c = rp(yIdx*28+xIdx);
          float x = (i+2)*30.0f + xIdx*scale;
          float y = 28.0f*scale - yIdx*scale;
          glColor3f(c, c, c);
          glVertex2f(x, y);
          glVertex2f(x+scale, y);
          glVertex2f(x+scale, y+scale);
          glVertex2f(x, y+scale);
        }
      }
      glEnd();
    }

    for(int i = 0; i < visualizedNeurons; i++)
    {
      glBegin(GL_QUADS);
      double mi = rbm.W.row(i).minCoeff();
      double ma = rbm.W.row(i).maxCoeff();
      double range = ma-mi;
      for(int xIdx = 0; xIdx < 28; xIdx++)
      {
        for(int yIdx = 0; yIdx < 28; yIdx++)
        {
          float c = (rbm.W(i+offset, yIdx*28+xIdx) - mi) / range;
          float x = i*30.0f*scale + xIdx*scale;
          float y = 30.0f + (28.0f-yIdx)*scale;
          glColor3f(c, c, c);
          glVertex2f(x, y);
          glVertex2f(x+scale, y);
          glVertex2f(x+scale, y+scale);
          glVertex2f(x, y+scale);
        }
      }
      glEnd();
    }

    glColor3f(0.0f,0.0f,0.0f);
    renderText(10, 35, "MNIST data set", QFont("Helvetica", 20));

    glFlush();
  }

  virtual void keyPressEvent(QKeyEvent* keyEvent)
  {
    switch(keyEvent->key())
    {
      case Qt::Key_Up:
        instance++;
        if(instance >= dataSet.samples())
          instance = dataSet.samples()-1;
        update();
        break;
      case Qt::Key_Down:
        instance--;
        if(instance < 0)
          instance = 0;
        update();
        break;
      case Qt::Key_Left:
        offset--;
        if(offset < 0)
          offset = 0;
        update();
        break;
      case Qt::Key_Right:
        offset++; // TODO we could reach to max. hidden unit
        update();
        break;
      case Qt::Key_Minus:
        fantasy--;
        if(fantasy < 0)
          fantasy = 0;
        update();
        break;
      case Qt::Key_Plus:
        fantasy++;
        update();
        break;
      default:
        QGLWidget::keyPressEvent(keyEvent);
        break;
    }
  }
};

int main(int argc, char** argv)
{
#ifdef PARALLEL_CORES
  omp_set_num_threads(PARALLEL_CORES);
#endif
  OpenANN::Logger logger(OpenANN::Logger::CONSOLE);
  OpenANN::Logger parameterLogger(OpenANN::Logger::APPEND_FILE, "rbm-weights");

  QApplication app(argc, argv);

  std::string directory = "mnist/";
  if(argc > 1)
    directory = std::string(argv[1]);

  IDXLoader loader(28, 28, 60000, 1000, directory);
  OpenANN::DirectStorageDataSet trainSet(&loader.trainingInput);
  //OpenANN::DirectStorageDataSet testSet(loader.testInput);

  OpenANN::RBM rbm(784, 10, 1, 0.01);
  rbm.trainingSet(trainSet);

  OpenANN::MBSGD optimizer(0.01, 0.5, 50, 0.1);
  OpenANN::StoppingCriteria stop;
  stop.maximalIterations = 20;
  optimizer.setOptimizable(rbm);
  optimizer.setStopCriteria(stop);
  int it = 0;
  while(optimizer.step())
  {
    OPENANN_INFO << "Iteration #" << ++it << " finished.";
  }
  parameterLogger << rbm.currentParameters();

  RBMVisualization visual(rbm, trainSet, 5, 800, 600);
  visual.show();
  visual.resize(800, 600);

  return app.exec();
}
