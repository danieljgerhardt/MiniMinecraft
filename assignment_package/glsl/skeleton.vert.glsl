#version 150
// ^ Change this to version 130 if you have compatibility issues

//This is a vertex shader. While it is called a "shader" due to outdated conventions, this file
//is used to apply matrix transformations to the arrays of vertex data passed to it.
//Since this code is run on your GPU, each vertex is transformed simultaneously.
//If it were run on your CPU, each vertex would have to be processed in a FOR loop, one at a time.
//This simultaneous transformation allows your program to run much faster, especially when rendering
//geometry with millions of vertices.

uniform mat4 u_Model;       // The matrix that defines the transformation of the
                            // object we're rendering. In this assignment,
                            // this will be the result of traversing your scene graph.

uniform mat4 u_ModelInvTr;  // The inverse transpose of the model matrix.
                            // This allows us to transform the object's normals properly
                            // if the object has been non-uniformly scaled.

uniform mat4 u_ViewProj;    // The matrix that defines the camera's transformation.
                            // We've written a static matrix for you to use for HW2,
                            // but in HW3 you'll have to generate one yourself

uniform mat4[100] u_Binds;          // arr of bind matrices
uniform mat4[100] u_Transforms;     // arr of transformation matrices

in vec4 vs_Pos;             // The array of vertex positions passed to the shader

in vec4 vs_Nor;             // The array of vertex normals passed to the shader

in vec4 vs_Col;             // The array of vertex colors passed to the shader.

in ivec2 vs_Ids;            // ids of joints
in vec2 vs_Weights;         // weights of joints

out vec3 fs_Pos;
out vec4 fs_Nor;            // The array of normals that has been transformed by u_ModelInvTr. This is implicitly passed to the fragment shader.
out vec4 fs_Col;            // The color of each vertex. This is implicitly passed to the fragment shader.

#define K vec3(0,0,0)
#define B vec3(0,0,1)
#define G vec3(0,1,0)
#define Y vec3(1,1,0)
#define R vec3(1,0,0)
#define W vec3(1,1,1)
vec3 heatMap(float t) {
    if (t < 0.167) {
        return mix(K, B, t/0.167);
    } else if (t < 0.33) {
        return mix(B, G, (t-0.167)/0.167);
    } else if (t < 0.5) {
        return mix(G, Y, (t-0.33)/0.167);
    } else if (t < 0.667) {
        return mix(Y, R, (t-0.5)/0.167);
    } else {
        return mix(R, W, (t-0.667)/0.167);
    }
}

void main()
{
    fs_Col = vs_Col;                         // Pass the vertex colors to the fragment shader for interpolation
    //fs_Col = ((u_Transforms[0][3]) + vec4(1, 1, 1, 1)) * 0.5;
    //fs_Col = vec4(heatMap(vs_Weights.r), 1);
    //fs_Col = vec4(vs_Weights.xy, 0.5, 1);

    mat3 invTranspose = mat3(u_ModelInvTr);
    fs_Nor = vec4(invTranspose * vec3(vs_Nor), 0);          // Pass the vertex normals to the fragment shader for interpolation.
                                                            // Transform the geometry's normals by the inverse transpose of the
                                                            // model matrix. This is necessary to ensure the normals remain
                                                            // perpendicular to the surface after the surface is transformed by
                                                            // the model matrix.


    /*vec4 jointAPos = u_Transforms[vs_Ids.x] * u_Binds[vs_Ids.x] * vs_Pos;
    vec4 jointBPos = u_Transforms[vs_Ids.y] * u_Binds[vs_Ids.y] * vs_Pos;
    vec4 modelposition = u_Model * (vs_Weights.x * jointAPos + vs_Weights.y * jointBPos);*/

    /*vec4 modelposition =  u_Model * ((vs_Weights.x * u_Transforms[vs_Ids.x] * u_Binds[vs_Ids.x] * vs_Pos) +
                (vs_Weights.y * u_Transforms[vs_Ids.y] * u_Binds[vs_Ids.y] * vs_Pos));*/

    vec4 modelposition =  u_Model * ((1 * u_Transforms[vs_Ids.x] * u_Binds[vs_Ids.x] * vs_Pos) +
                (0 * u_Transforms[vs_Ids.y] * u_Binds[vs_Ids.y] * vs_Pos));


    fs_Pos = modelposition.xyz;

    gl_Position = u_ViewProj * modelposition;// gl_Position is a built-in variable of OpenGL which is
                                             // used to render the final positions of the geometry's vertices
}
