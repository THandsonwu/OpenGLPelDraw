//
//  main.cpp
//  OpenGL图元绘制
//
//  Created by tanzhiwu on 2020/7/10.
//  Copyright © 2020 tanzhiwu. All rights reserved.
//

#include "GLTools.h"
#include "GLMatrixStack.h" //矩阵的工具类,可以利于GLMatrixStack 加载单元矩阵/矩阵相乘/压栈/出栈/缩放/平移/旋转
#include "GLFrame.h"   //矩阵工具类,表示位置,通过设置vOrigin, vForward, vUp等
#include "GLFrustum.h" //矩阵工具类,用来快速设置 正/透视投影矩阵,完成坐标从3D->2D映射过程
#include "GLBatch.h"   // 三角形批次类,帮助类,利用它可以传输顶点/光照/纹理/颜色数据到存储着色器中
#include "GLGeometryTransform.h" //变换管道类,用来快速在代码中传输视图矩阵/投影矩阵/视图投影变换矩阵等


#include <math.h>
#ifdef __APPLE__
#include <glut/glut.h>
#else
#define FREEGLUT_STATIC
#include <GL/glut.h>
#endif


#pragma mark  ---- 定义属性

//存储着色器管理器
GLShaderManager       shaderManager;
//模型视图矩阵
GLMatrixStack         modelViewMatrix;
//投影矩阵
GLMatrixStack         projectionMatrix;
//管擦着视图坐标
GLFrame               cameraFrame;
//设置图形环绕时,视图坐标
GLFrame               objectFrame;
//设置图元绘制时的投影方式
GLFrustum             viewFrustum;
//点图元容器
GLBatch               pointBatch;
//线段图元容器
GLBatch               lineBatch;
//连线图元容器
GLBatch               lineStripBatch;
//闭合连线图元容器
GLBatch               lineLoopBatch;
//三角形图元容器
GLBatch               triangleBatch;
//共用顶点的三角形图元容器
GLBatch               triangleStripBatch;
//围绕一个圆点,共用相邻顶点的三角形图元容器
GLBatch               triangleFanBatch;

//几何变换管道
GLGeometryTransform   transformPipeline;


GLfloat vGreen[] = {0.0f, 1.0f, 0.0f, 1.0f};//绿色
GLfloat vRed[]   = {1.0f, 0.0f, 0.0f, 1.0f};//红色
GLfloat vBlack[] = {0.0f, 0.0f, 0.0f, 1.0f};//黑色

//跟踪效果步骤 ,记录用户按空格的次数,用来显示渲染不同的图形
int nStep = 0;

#pragma mark  ---- 绘制函数
void DrawWireFrameBatch(GLBatch *vBatch)
{
    //参数列表参考:http://blog.csdn.net/augusdi/article/details/23747081
    shaderManager.UseStockShader(GLT_SHADER_FLAT,transformPipeline.GetModelViewProjectionMatrix(),vGreen);
    vBatch->Draw();
    
    //画黑色边框
    //偏移深度,在同一位置要绘制填充和边线,会在z产生冲突,所以要偏移
    glPolygonOffset(-1.0f, -1.0f);
    glEnable(GL_POLYGON_OFFSET_LINE);
    
    //画反锯齿,让黑边丝滑
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    //绘制线框几何黑色版,三种模式,实心,边框,点,可以作用在正面,背面,或者两面
    //通过调用glPolygonMode将多边形正面或者背面设为线框模式,实现线框渲染
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    //设置线条宽度
    glLineWidth(5.0f);
    
    //绘制平面着色
    shaderManager.UseStockShader(GLT_SHADER_FLAT, transformPipeline.GetModelViewProjectionMatrix(),vBlack);
    vBatch->Draw();
    
    
    //复原原本的设置
    //通过调用glPolygonMode将多边形正面或者背面设为全部填充模式
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_POLYGON_OFFSET_LINE);
    glLineWidth(1.0f);
    glDisable(GL_BLEND);
    glDisable(GL_LINE_SMOOTH);
    
}


#pragma mark  ---- 重要的注册函数

/// 此函数在呈现上下文中进行任何必要的初始化
void SetupRC()
{
    
    //设置灰色背景
    glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
    //初始化管理器
    shaderManager.InitializeStockShaders();
    //开启深度测试
    glEnable(GL_DEPTH_TEST);
    //设置变换管线以下使用两个矩阵堆栈
    transformPipeline.SetMatrixStacks(modelViewMatrix, projectionMatrix);
    //设置视角
    cameraFrame.MoveForward(-15.0f);
    
    
    // 定义三个顶点
    GLfloat vCoast[9] = {
        3, 3, 0,
        0, 3, 0,
        3, 0, 0
    };
    
    
    //用点的方式
    //参数1 :表示使用的图元样式
    //参数2 :顶点数
    //参数3 :纹理坐标(可选)...
    pointBatch.Begin(GL_POINTS, 3);
    //复制顶点坐标进内存
    pointBatch.CopyVertexData3f(vCoast);
    //结束,表示已经完成数据复制工作
    pointBatch.End();
    
    
    //通过线的方式
    lineBatch.Begin(GL_LINES, 3);
    lineBatch.CopyVertexData3f(vCoast);
    lineBatch.End();
    
    
    //通过线段的形式
    lineStripBatch.Begin(GL_LINE_STRIP, 3);
    lineStripBatch.CopyVertexData3f(vCoast);
    lineStripBatch.End();
    
    
    //通过线环的样式
    lineLoopBatch.Begin(GL_LINE_LOOP, 3);
    lineLoopBatch.CopyVertexData3f(vCoast);
    lineLoopBatch.End();
    
    
    //通过三角形创建金字塔
    GLfloat vPyramid[12][3] = {
        -2.0f, 0.0f, -2.0f,
        2.0f,  0.0f, -2.0f,
        0.0f,  4.0f, 0.0f,
        
        
        2.0f, 0.0f, -2.0f,
        2.0f, 0.0f, 2.0f,
        0.0f, 4.0f, 0.0f,
        
        
        2.0f, 0.0f, 2.0f,
        -2.0f, 0.0f, 2.0f,
        0.0f, 4.0f, 0.0f,
        
        
        -2.0f, 0.0f, 2.0f,
        -2.0f, 0.0f, -2.0f,
        0.0f, 4.0f, 0.0f
    };
    
    
    //通过GL_TRIANGLES 每三个顶点定义一个新的三角形
    triangleBatch.Begin(GL_TRIANGLES, 12);
    triangleBatch.CopyVertexData3f(vPyramid);
    triangleBatch.End();
    
    
    //三角形扇形--六边形
    
    GLfloat vPoints[100][3];
    int nVerts = 0;
    //设置半径
    GLfloat r = 3.0f;
    //设置原点
    vPoints[nVerts][0] = 0.0f;
    vPoints[nVerts][1] = 0.0f;
    vPoints[nVerts][2] = 0.0f;
    
    
    for (GLfloat angle = 0; angle < M3D_2PI; angle += M3D_2PI / 6.0f) {
        //数组下标自增(每自增一次就表示一个顶点)
        nVerts++;
        /*
         弧长=半径*角度,这里的角度是弧度制,不是平时的角度制
         既然知道了cos值,那么角度=arccos,求一个反三角函数就行了
         */
        //x点坐标 cos(angle) * 半径
        vPoints[nVerts][0] = float(cos(angle)) * r;
        //y点坐标 sin(angle) * 半径
        vPoints[nVerts][1] = float(sin(angle)) * r;
        //z点的坐标
        vPoints[nVerts][2] = -1.0f;
    }
    
    //结束扇形,前面一共绘制7个顶点(包含圆心),不添加闭合点,则三角扇形是无法闭合的。
    nVerts++;
    vPoints[nVerts][0] = r;
    vPoints[nVerts][1] = 0;
    vPoints[nVerts][2] = 0.0f;
    
    
    //加载,GL_TRIANGLE_FAN,以一个圆心为中心呈扇形排列,共用相邻顶点的一组三角形
    triangleFanBatch.Begin(GL_TRIANGLE_FAN, 8);
    triangleFanBatch.CopyVertexData3f(vPoints);
    triangleFanBatch.End();
    
    
    //三角形条带,圆柱环
    //顶点下标
    int iCounter = 0;
    //半径
    GLfloat radius = 3.0f;
    //从 0 度~ 360度,以0.3弧度为步长
    for (GLfloat angle = 0.0f; angle <= (2.0f * M3D_PI); angle += 0.3f) {
        //获取圆形的顶点X,Y
        GLfloat x = radius *sin(angle);
        GLfloat y = radius *cos(angle);
        
        //绘制拼接的两个三角形(它们的想,y顶点一样,只有z点不一样)
        vPoints[iCounter][0] = x;
        vPoints[iCounter][1] = y;
        vPoints[iCounter][2] = -0.5;
        iCounter++;
        
        
        vPoints[iCounter][0] = x;
        vPoints[iCounter][1] = y;
        vPoints[iCounter][2] = 0.5;
        iCounter++;
        
    }
    
    //结束循环,在循环末尾生成两个拼接三角形
    vPoints[iCounter][0] = vPoints[0][0];
    vPoints[iCounter][1] = vPoints[0][1];
    vPoints[iCounter][2] = -0.5;
    iCounter++;
       
    vPoints[iCounter][0] = vPoints[1][0];
    vPoints[iCounter][1] = vPoints[1][1];
    vPoints[iCounter][2] = 0.5;
    iCounter++;
    
    // 加载 GL_TRIANGLE_STRIP 共用一个条带上的顶点的一组三角形
    triangleStripBatch.Begin(GL_TRIANGLE_STRIP, iCounter);
    triangleStripBatch.CopyVertexData3f(vPoints);
    triangleStripBatch.End();
    
    
}


/// 召唤渲染,每次有改变都要重新渲染
void RenderScene()
{
    //清空当前画布,每次渲染之前必须做
    //GL_COLOR_BUFFER_BIT   :颜色
    //GL_DEPTH_BUFFER_BIT   :深度
    //GL_STENCIL_BUFFER_BIT :模版
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    
    //压栈
    modelViewMatrix.PushMatrix();
    M3DMatrix44f mCamera;
    //获取视角矩阵
    cameraFrame.GetCameraMatrix(mCamera);
    //矩阵乘以矩阵堆栈的顶部矩阵,相乘的结果随后就存储在堆栈的顶部
    modelViewMatrix.MultMatrix(mCamera);
    
    
    M3DMatrix44f mObjectFrame;
    //只要使用 GetMatrix 函数就可以可以获取矩阵堆栈顶部的值,这个海曙可以进行二次重载。
    //可以用来给GLShaderManager的使用
    //可以获取顶部矩阵的顶点副本数据
    objectFrame.GetCameraMatrix(mObjectFrame);
    modelViewMatrix.MultMatrix(mObjectFrame);
    
    
    /*
     参数1:平面着色器
     参数2:运行为几何图形变换指定一个4 * 4变换矩阵
     参数3:颜色值,红色
     */
    shaderManager.UseStockShader(GLT_SHADER_FLAT, transformPipeline.GetModelViewProjectionMatrix(),vRed);
    
    
    switch (nStep) {
        case 0:
            glPointSize(10.0f);//设置点大小
            pointBatch.Draw();
            glPointSize(1.0f);//恢复原来大小
            break;
        case 1:
            glLineWidth(5.0f);//设置线宽
            lineBatch.Draw();
            glLineWidth(1.0f);
        
            break;
        case 2:
            glLineWidth(5.0f);
            lineStripBatch.Draw();
            glLineWidth(1.0f);
            break;
        case 3:
            glLineWidth(5.0f);
            lineLoopBatch.Draw();
            glLineWidth(1.0f);
            break;
        case 4:
            DrawWireFrameBatch(&triangleBatch);
            break;
        case 5:
            DrawWireFrameBatch(&triangleStripBatch);
            break;
        case 6:
            DrawWireFrameBatch(&triangleFanBatch);
            break;
            
        default:
            break;
    }
    //出栈,还原到以前的视图模型矩阵(单位矩阵)
    modelViewMatrix.PopMatrix();
    
    //进行缓冲区交换
    glutSwapBuffers();
    
}


/// 特殊键位处理(上,下,左,右移动)
void SpecialKeys(int key, int x, int y)
{
    if (key == GLUT_KEY_UP) {
        //围绕一个指定的X,Y,Z轴旋转
        objectFrame.RotateWorld(m3dDegToRad(-5.0f), 1.0f, 0.0f, 0.0f);
    }
    if (key == GLUT_KEY_DOWN) {
        //围绕一个指定的X,Y,Z轴旋转
        objectFrame.RotateWorld(m3dDegToRad(5.0f), 1.0f, 0.0f, 0.0f);
    }
    if (key == GLUT_KEY_LEFT) {
        //围绕一个指定的X,Y,Z轴旋转
        objectFrame.RotateWorld(m3dDegToRad(-5.0f), 0.0f, 1.0f, 0.0f);
    }
    if (key == GLUT_KEY_RIGHT) {
        //围绕一个指定的X,Y,Z轴旋转
        objectFrame.RotateWorld(m3dDegToRad(5.0f), 0.0f, 1.0f, 0.0f);
    }
    glutPostRedisplay();//开启重绘
    
}

//根据空格次数,切换不同的"窗口名称"
void KeyPressFunc(unsigned char key, int x, int y)
{
   if(key == 32)
    {
        nStep++;
        
        if(nStep > 6)
            nStep = 0;
    }
    
    switch(nStep)
    {
        case 0:
            glutSetWindowTitle("GL_POINTS");
            break;
        case 1:
            glutSetWindowTitle("GL_LINES");
            break;
        case 2:
            glutSetWindowTitle("GL_LINE_STRIP");
            break;
        case 3:
            glutSetWindowTitle("GL_LINE_LOOP");
            break;
        case 4:
            glutSetWindowTitle("GL_TRIANGLES");
            break;
        case 5:
            glutSetWindowTitle("GL_TRIANGLE_STRIP");
            break;
        case 6:
            glutSetWindowTitle("GL_TRIANGLE_FAN");
            break;
    }
    
    glutPostRedisplay();
}

//窗口更改大小,或者刚刚创建,无论哪种情况,我们都需要
//使用窗口维度设置视口和投影矩阵
void ChangeSize(int w, int h)
{
    glViewport(0, 0, w, h);
    //创建投影矩阵,并将它载入投影矩阵堆栈中
    viewFrustum.SetPerspective(35.0f, float(w)/float(h), 1.0f, 500.f);
    projectionMatrix.LoadMatrix(viewFrustum.GetProjectionMatrix());
    //调用顶部载入单元矩阵
    modelViewMatrix.LoadIdentity();
}

#pragma mark ---- 主执行函数

int main(int argc, char* argv[])
{
    gltSetWorkingDirectory(argv[0]);
    glutInit(&argc, argv);
    //申请一个颜色缓存区,深度缓存区,双缓存区,模版缓存区
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGBA|GLUT_DEPTH|GLUT_STENCIL);
    //设置window的尺寸
    glutInitWindowSize(900, 600);
    //创建window名称
    glutCreateWindow("GL_POINTS");
    //注册尺寸改变的回调函数
    glutReshapeFunc(ChangeSize);
    //注册点击空格的回调函数
    glutKeyboardFunc(KeyPressFunc);
    //注册特殊键位回调函数(上下左右)
    glutSpecialFunc(SpecialKeys);
    //注册渲染刷新显示函数
    glutDisplayFunc(RenderScene);
    
    //判断是否能初始化glew库,确保项目能正常使用OpenGL框架
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        fprintf(stderr, "GLEW Error: %s\n",glewGetErrorString(err));
        return -999;
    }
    //初始化绘制
    SetupRC();
    //开始runloop运行循环
    glutMainLoop();
    return 0;
}






