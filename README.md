自己实现cpu光栅化的tinyrenderer。
坐标系与相机设置unity内置渲染管线的对齐。
实现了MVP变换（模型未移动，即模型空间为单位阵）、zbuffer、顶点/片元着色器、blinn-phong(phong)、uv normal、uv texture、shadow mapping

可以通过cmake构建。
相机位置和光源位置通过main.cpp中的constexpr vec3 eye{-1, 0, 3};和constexpr vec3 light_start_point = {1, 1, 1};指定。

* 正交投影+模型wire画线
![渲染结果](imgs/1.Wire_framebuffer.png)
* 正交投影+模型三角面光栅化
![渲染结果](imgs/2.raste_framebuffer.png)
* 透视投影+phong shading
![渲染结果](imgs/3.phong_framebuffer.png)
* 透视投影+phong shading+uv normal+uv texture
![渲染结果](imgs/4.phong_tex_uv_framebuffer.png)
* 透视投影+blinn-phong shading+uv normal+uv texture
![渲染结果](imgs/5.blinn_phong_tex_uv_framebuffer.png)
* 透视投影+blinn-phong shading+uv normal+uv texture+shadow mapping
![渲染结果](imgs/6.blinn_phong_tex_uv_shadowmapping_framebuffer.png)
