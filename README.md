### Interpolation Curve, Bezier C1 Curves, Bezier 6-th Degree Curve And Bezier Surface using `OpenGL`.

This program displays three different curves and one surface, specifically when running the executable the given choices are given with the right click menu:
1. **An interpolation curve**. To draw this curve, a transformation matrix was inserted, to convert the control points of the desired interpolation curve to control points of the equal bezier curve. OpenGL does not support functions for interpolation curves.
2. **A 6-th degree Bezier curve** with 7 control points.
3. **Two 3-rd degree Bezier curves** with C1 continuity.
4. **A bezier surface**, where the corner control points have their x-coordinate equal to 0 and the other control points have their x-coordinate equal to 20. The surface can rotate with the left and right arrows of the keyboard. 

In the first 3 options the control points can be moved using the left click of the mouse.

Source code and windows executable are included.
The program was created during a university project.
