#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <shader.h>
#include <camera.h>
#include <model.h>

#include <stb_image.h>

#include <iostream>

#include <irrklang/irrKlang.h>
using namespace irrklang;


//define all texture beforehand
struct Textures
{
    unsigned int woodTexture;
    unsigned int cubeDiffuse;
    unsigned int cubeSpecular;
    unsigned int wallSpecular;
    unsigned int rockTexture;
} textures;

//func
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
glm::vec3 move_to_pos(glm::vec3 position, glm::vec3 endPoint, float speed);
unsigned int loadTexture(char const* path);
void renderQuad();
void renderCube();
void renderPlane();
void renderScene(const Shader& shader, Textures& textures);
void initPlanet(const Shader& shader);
void wildTransforms(const Shader& shader, Textures& textures);
void drawLamps(const Shader& lampShader, glm::vec3 pointLightPos[], glm::vec3 pointLightColors[]);
unsigned int loadCubemap(vector<std::string> faces);
void renderSkyBox();

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 0.4f, 5.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float runTime = 0.0f;

//debug
bool wireframe;



int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Reverie Beyond Stars", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    //glfwSetCursorPosCallback(window, mouse_callback);
    //glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    //glDepthFunc(GL_LESS);
    

    //flip textures y axis
    //stbi_set_flip_vertically_on_load(true);


    // build shaders
    // ------------------------------------
    Shader shader("shaders/project.vs", "shaders/project.fs");
    Shader lampShader("shaders/light_cube.vs", "shaders/light_cube.fs");
    Shader hdrShader("shaders/hdr.vs", "shaders/hdr.fs");
    Shader wildShader("shaders/wild.vs", "shaders/wild.fs");
    Shader skyBoxShader("shaders/skybox.vs", "shaders/skybox.fs");

    unsigned int hdrFBO;
    glGenFramebuffers(1, &hdrFBO);
    // create floating point color buffer
    unsigned int colorBuffer;
    glGenTextures(1, &colorBuffer);
    glBindTexture(GL_TEXTURE_2D, colorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // create depth buffer (renderbuffer)
    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    // attach buffers
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffer, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    //textures
    unsigned int woodTexture = loadTexture("textures/floor.png");
    unsigned int cubeDiffuse = loadTexture("textures/container2.png");
    unsigned int cubeSpecular = loadTexture("textures/container2_specular.png");
    unsigned int wallSpecular = loadTexture("textures/brickwall_specular.jpg");
    unsigned int rockTexture = loadTexture("textures/rock.jpg");

    //skybox
    vector<std::string> faces
    {
        "textures/skybox/right.png",
        "textures/skybox/left.png",
        "textures/skybox/top.png",
        "textures/skybox/bottom.png",
        "textures/skybox/front.png",
        "textures/skybox/back.png"
    };
    unsigned int skyBoxTexture = loadCubemap(faces);

    //model
    Model planet("models/planet/planet.obj");

    //remember to define new textures to the struct
    textures.woodTexture = woodTexture;
    textures.cubeDiffuse = cubeDiffuse;
    textures.cubeSpecular = cubeSpecular;
    textures.wallSpecular = wallSpecular;
    textures.rockTexture = rockTexture;


    //set up shaders
    //basic shader
    shader.use();
    shader.setInt("material.diffuse", 0);
    shader.setInt("material.specular", 1);
    shader.setFloat("material.shininess", 64.0f);

    //lights
    glm::vec3 pointLightPos[] = {
        glm::vec3(5.f,  0.5f,  5.0f),
        glm::vec3(-5.0f, 3.3f, -4.0f),
        glm::vec3(-4.0f,  2.0f, 5.0f),
    };

    glm::vec3 pointLightColors[] = {
        glm::vec3(1.8f,  1.8f,  1.8f),
        glm::vec3(1.8f, 1.8f, 1.8f),
        glm::vec3(1.8f,  1.8f, 1.8f),
    };


    glm::vec3 diffuse = glm::vec3(1.8f, 1.8f, 1.8f);

    // directional light
    shader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
    shader.setVec3("dirLight.ambient", 0.02f, 0.02f, 0.02f);
    shader.setVec3("dirLight.diffuse", 0.2f, 0.2f, 0.2f);
    shader.setVec3("dirLight.specular", 0.1f, 0.1f, 0.1f);

    // point light 1
    shader.setVec3("pointLights[0].ambient", 0.05f, 0.05f, 0.05f);
    shader.setVec3("pointLights[0].diffuse", diffuse);
    shader.setVec3("pointLights[0].specular", 1.0f, 1.0f, 1.0f);
    shader.setFloat("pointLights[0].constant", 1.0f);
    shader.setFloat("pointLights[0].linear", 0.09);
    shader.setFloat("pointLights[0].quadratic", 0.032);

    // point light 2
    shader.setVec3("pointLights[1].ambient", 0.05f, 0.05f, 0.05f);
    shader.setVec3("pointLights[1].diffuse", diffuse);
    shader.setVec3("pointLights[1].specular", 1.0f, 1.0f, 1.0f);
    shader.setFloat("pointLights[1].constant", 1.0f);
    shader.setFloat("pointLights[1].linear", 0.09);
    shader.setFloat("pointLights[1].quadratic", 0.032);

    // point light 3
    shader.setVec3("pointLights[2].ambient", 0.05f, 0.05f, 0.05f);
    shader.setVec3("pointLights[2].diffuse", diffuse);
    shader.setVec3("pointLights[2].specular", 1.0f, 1.0f, 1.0f);
    shader.setFloat("pointLights[2].constant", 1.0f);
    shader.setFloat("pointLights[2].linear", 0.09);
    shader.setFloat("pointLights[2].quadratic", 0.032);



    //spotlight
    glm::vec3 spotLightPos = glm::vec3(10.0f, 18.0f, 22.0f);

    shader.setVec3("spotLight.position", spotLightPos);
    shader.setVec3("spotLight.direction", glm::vec3(1.0f, 1.0f, 1.0f));
    shader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
    shader.setVec3("spotLight.diffuse", 5.0f, 5.0f, 5.0f);
    shader.setVec3("spotLight.specular", 0.1f, 0.1f, 0.1f);
    shader.setFloat("spotLight.constant", 1.0f);
    shader.setFloat("spotLight.linear", 0.09);
    shader.setFloat("spotLight.quadratic", 0.032);
    shader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
    shader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(25.0f)));

    //hdr shader
    hdrShader.use();
    hdrShader.setInt("hdrBuffer", 0);

    //wild shader
    wildShader.use();
    wildShader.setInt("tex", 0);

    //skyBox
    skyBoxShader.use();
    skyBoxShader.setInt("skybox", 0);

    //music
    ISoundEngine* SoundEngine = createIrrKlangDevice();
    SoundEngine->play2D("music/levelcomplete.wav", true);

    //vars
    glm::vec3 cameraTarget = glm::vec3(20.0f, 20.0f, 20.0f);
    glm::vec3 cameraTarget2 = glm::vec3(0.0f, 0.0f, 0.0f);
    float radius = 5.0f;
    float interval = 0.001f;
    float theta = 0.0f;
    float alpha = 0.0f;
    float angle = 5.0f;

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        runTime = runTime + deltaTime;

        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(0.00f, 0.00f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //render to framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        //set up
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 model = glm::mat4(1.0f);
   
        shader.use();
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        shader.setVec3("viewPos", camera.Position);

        
        


        //scripts

        //view planet sunrise
        if (runTime < 20.0)
        {
            view = glm::lookAt(camera.Position, glm::vec3(20.0f, 20.0f, 20.0f), camera.WorldUp);       
        }
        
        //Watch wild transform
        if (runTime > 20 && runTime < 90)
        {
            if (cameraTarget != glm::vec3(0.0f, 0.0f, 0.0f))
            { 
                cameraTarget = move_to_pos(cameraTarget, glm::vec3(0.0f, 0.0f, 0.0f), 0.2f);
                view = glm::lookAt(camera.Position, cameraTarget, camera.WorldUp);
            }
            else
            {
                float xPos = radius * cos(theta);
                float zPos = radius * sin(theta);
                camera.Position.x = zPos * cos(alpha) - xPos * sin(alpha);
                camera.Position.z = xPos * cos(alpha) + zPos * sin(alpha);
                alpha += interval;

                view = glm::lookAt(camera.Position, cameraTarget, camera.WorldUp);
            }



            //wild transform
            wildShader.use();
            wildShader.setMat4("projection", projection);
            wildShader.setMat4("view", view);
            wildTransforms(wildShader, textures);
        }

        //lights and scene
        if (runTime > 90 && runTime < 150)
        {
            camera.Position = move_to_pos(camera.Position, glm::vec3(6.0f, 2.5f, 6.0f), 0.1f);
            view = glm::lookAt(camera.Position, glm::vec3(0.0f, 3.5f, 0.0f), camera.WorldUp);
            shader.use();
            shader.setMat4("view", view);
            shader.setVec3("viewPos", camera.Position);

            //lights
            pointLightPos[0] = glm::vec3(
                sin(runTime) * 1.5f, sin(runTime) + 4.0f, cos(runTime) * 1.0f
            );
            pointLightPos[1] = glm::vec3(
                cos(runTime) * 1.5f, sin(-runTime) * 2.5f + 2.0f, sin(-runTime) * 2.5f
            );
            pointLightPos[2] = glm::vec3(
                sin(-runTime) * 4.5f, cos(runTime) * 1.5f + 2.5f, cos(-runTime) * 4.5f
            );
            
            //colors
            if (runTime > 120)
            {
                float color1 = (sin(runTime) + 1.0f) * 0.5f;
                float color2 = (cos(runTime) + 1.2f) * 0.5f;
                float color3 = (sin(-runTime) + 1.1f) * 0.5f;
                float color4 = (sin(-runTime) + 1.1f) * 0.5f;

                pointLightColors[0] = glm::vec3(color1, color2, color3);
                pointLightColors[1] = glm::vec3(color4, color3, color1);
                pointLightColors[2] = glm::vec3(color2, color1, color4);
            }


            shader.setVec3("pointLights[0].position", pointLightPos[0]);
            shader.setVec3("pointLights[1].position", pointLightPos[1]);
            shader.setVec3("pointLights[2].position", pointLightPos[2]);
            shader.setVec3("pointLights[0].diffuse", pointLightColors[0]);
            shader.setVec3("pointLights[1].diffuse", pointLightColors[1]);
            shader.setVec3("pointLights[2].diffuse", pointLightColors[2]);
            shader.setVec3("pointLights[0].specular", pointLightColors[0] * 1.2f);
            shader.setVec3("pointLights[1].specular", pointLightColors[1] * 1.2f);
            shader.setVec3("pointLights[2].specular", pointLightColors[2] * 1.2f);
            shader.setFloat("material.shininess", 86.0f);

            //scene
            renderScene(shader, textures);


            //lamps
            lampShader.use();
            lampShader.setMat4("projection", projection);
            lampShader.setMat4("view", view);
            drawLamps(lampShader, pointLightPos, pointLightColors);
        }


        if (runTime > 150) {
            cameraTarget2 = move_to_pos(cameraTarget2, glm::vec3(20.0f, 20.0f, 20.0f), 0.2f);
            view = glm::lookAt(camera.Position, cameraTarget2, camera.WorldUp);
        }


        if (runTime > 162)
            glfwSetWindowShouldClose(window, true);
        

        //draw planet
        shader.use();
        shader.setMat4("view", view);
        initPlanet(shader);
        planet.Draw(shader);

        //skybox
        glDepthFunc(GL_LEQUAL);
        skyBoxShader.use();
        skyBoxShader.setMat4("view", view);
        skyBoxShader.setMat4("projection", projection);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skyBoxTexture);
        renderSkyBox();
        glDepthFunc(GL_LESS);
        
        
        

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        //render framebuffer and convert to hdr
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        hdrShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffer);
        renderQuad();


        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    /*
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

    */
    if (glfwGetKey(window, GLFW_KEY_F4) == GLFW_PRESS) {
        glPolygonMode(GL_FRONT_AND_BACK, (wireframe = not wireframe) ? GL_FILL : GL_LINE);
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; 

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}


//move object from current position towards endpoint
//you have to initialize position vector for the object beforehand and use it as and argument to update itself
//Ex. objectPos = move_to_pos(objectPos, glm::vec3(2.0f, 2.0f, 2.0f), 0.01f);
glm::vec3 move_to_pos(glm::vec3 position, glm::vec3 endPoint, float speed) 
{
    glm::vec3 velocity = glm::vec3(0.0f);
    //distance between the position vectors
    glm::vec3 dist = (endPoint - position);
    //because of float margin of error, calculate check value for the distance between vectors: sum of it's components
    float check = abs(dist.x) + abs(dist.y) + abs(dist.z);

    //check against speed
    //if smaller than speed the object is within margin of error so it's moved to the end
    if (check < speed) {
        position = endPoint;
    }
    else {
        velocity = normalize(endPoint - position) * speed;
    }

    position += velocity;
    return position;
}

unsigned int loadTexture(char const* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        if (nrComponents == 3)
            format = GL_RGB;
        if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    }
    else 
    {
        std::cout << "Texture failed to load " << path << std::endl;
    }

    stbi_image_free(data);
    return textureID;
}

void renderScene(const Shader& shader, Textures& textures)
{
    glm::mat4 model = glm::mat4(1.0f);
    
    //glActiveTexture(GL_TEXTURE0);
    //glBindTexture(GL_TEXTURE_2D, textures.wallDiffuse); //floor diffuse
    //glActiveTexture(GL_TEXTURE1);
    //glBindTexture(GL_TEXTURE_2D, textures.wallSpecular); //floor diffuse

    
    //shader.setMat4("model", model);
    //renderPlane();
    


    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textures.cubeDiffuse);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textures.cubeDiffuse);

    if (runTime > 120)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures.rockTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, textures.rockTexture);
    }

    //cube1
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(
        sin(runTime) * 2.0f, 1.5f, cos(runTime) * 2.0f
    ));
    model = glm::rotate(model, glm::radians(50.0f) * runTime, glm::vec3(0.0f, 1.0f, 1.0f));
    model = glm::scale(model, glm::vec3(0.5f));
    shader.setMat4("model", model);
    renderCube();

    //cube2
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(
        cos(runTime) * 3.0f, 3.2f, sin(runTime) * 3.0f
    ));
    model = glm::rotate(model, glm::radians(30.0f) * runTime, glm::vec3(1.0f));
    model = glm::scale(model, glm::vec3(0.6f));
    shader.setMat4("model", model);
    renderCube();

    //cube3
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(
        sin(-runTime) * 1.0f, 0.5f, cos(runTime) * 1.0f
    ));
    model = glm::rotate(model, glm::radians(60.0f) * runTime, glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
    model = glm::scale(model, glm::vec3(0.25f));
    shader.setMat4("model", model);
    renderCube();

    //cube4
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(
        cos(-runTime) * 2.2f, 5.8f, sin(-runTime) * 2.2f
    ));
    model = glm::rotate(model, glm::radians(60.0f) * runTime, glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
    model = glm::scale(model, glm::vec3(0.4f));
    shader.setMat4("model", model);
    renderCube();

    //cube5
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(
        sin(runTime) * 3.5f, 4.5f, cos(-runTime) * 3.5f
    ));
    model = glm::rotate(model, glm::radians(60.0f) * runTime, glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
    model = glm::scale(model, glm::vec3(0.4f));
    shader.setMat4("model", model);
    renderCube();
}


unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}


unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void renderCube()
{
    // initialize (if necessary)
    if (cubeVAO == 0)
    {
        float vertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            // front face
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
            // bottom face
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            // top face
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
             1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
             1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // render Cube
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}






unsigned int planeVAO = 0;
unsigned int planeVBO;
void renderPlane()
{
    if (planeVAO == 0)
    {
        float planeVertices[] = {
            // positions            // normals         // texcoords
            5.0f, -0.5f,  5.0f,  0.0f, 1.0f, 0.0f,  5.0f,  0.0f,
            -5.0f, -0.5f,  5.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
            -5.0f, -0.5f, -5.0f,  0.0f, 1.0f, 0.0f,   0.0f, 5.0f,

             5.0f, -0.5f,  5.0f,  0.0f, 1.0f, 0.0f,  5.0f,  0.0f,
            -5.0f, -0.5f, -5.0f,  0.0f, 1.0f, 0.0f,   0.0f, 5.0f,
             5.0f, -0.5f, -5.0f,  0.0f, 1.0f, 0.0f,  5.0f, 5.0f

        };
        // setup plane VAO
        glGenVertexArrays(1, &planeVAO);
        glGenBuffers(1, &planeVBO);
        glBindVertexArray(planeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindVertexArray(0);
    }
    glBindVertexArray(planeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

glm::vec3 spotLightPos = glm::vec3(25.0f, 20.0f, 30.0f);
void initPlanet(const Shader& shader) 
{
    if (runTime < 150)
    {
        spotLightPos = move_to_pos(spotLightPos, glm::vec3(10.0f, 20.0f, 25.0f), 0.02f);
    } 
    else
    {
        spotLightPos = move_to_pos(spotLightPos, glm::vec3(30.0f, 20.0f, 30.0f), 0.012f);
    }

    glm::vec3 planetPos = glm::vec3(20.0f, 20.0f, 20.0f);
    glm::vec3 spotLightDir = normalize(planetPos - spotLightPos);
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, planetPos);
    model = glm::rotate(model, glm::radians(10.0f) * runTime, glm::vec3(0.0f, 1.0f, 0.0f));

    shader.setVec3("spotLight.position", spotLightPos);
    shader.setVec3("spotLight.direction", spotLightDir);
    shader.setMat4("model", model);
}


void wildTransforms(const Shader& shader, Textures& textures)
{
    float a = sin(runTime);
    float b = cos(runTime);
    float c = -sin(runTime);

    glm::mat4 shear1 = glm::mat4(
        1.0f, 0.0f, a + b, 0.0f,
        0.0f, b,    0.0f,  0.0f,
        0.0f, c,    1.0f,  0.0f,
        0.0f, 0.0f, 0.0f,  1.0f);

    glm::mat4 shear2 = glm::mat4(
        a - b, b - c,   0.0f, 0.0f,
        0.0f,  1.0f,    b,    0.0f,
        c,  0.0f,    1.0f, 0.0f,
        0.0f,  0.0f,    0.0f, 1.0f);

    glm::mat4 shear3 = glm::mat4(
        1.0f,  c - b,   0.0f, 0.0f,
        a - b,     b,    a,    0.0f,
        0.0f,  0.0f,    1.0f, 0.0f,
        0.0f,  0.0f,    0.0f, 1.0f);

    
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(0.5f));
    model = glm::translate(model, glm::vec3(0.0f, 1.0f, 0.0f));

    if (runTime < 43.5f)
        shader.setMat4("shear", shear1);
    if (runTime > 43.5f && runTime < 65.2f)
        shader.setMat4("shear", shear2);
    if (runTime > 65.2f && runTime < 90.0f)
        shader.setMat4("shear", shear3);

    shader.setMat4("model", model);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textures.wallSpecular);
    renderCube();
}

void drawLamps(const Shader& lampShader, glm::vec3 pointLightPos[], glm::vec3 pointLightColors[])
{


    glm::mat4 model = glm::mat4(1.0f);

    for (unsigned int i = 0; i < 3; i++)
    {
        model = glm::mat4(1.0f);
        model = glm::translate(model, pointLightPos[i]);
        model = glm::scale(model, glm::vec3(0.2f));
        lampShader.setMat4("model", model);
        lampShader.setVec3("color", pointLightColors[i]);
        renderCube();
    }
}

unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

unsigned int skyboxVAO = 0;
unsigned int skyboxVBO;
void renderSkyBox() {

    if (skyboxVAO == 0)
    {
        float skyboxVertices[] = {
            // positions          
        -50.0f,  50.0f, -50.0f,
        -50.0f, -50.0f, -1.0f,
         50.0f, -50.0f, -50.0f,
         50.0f, -50.0f, -50.0f,
         50.0f,  50.0f, -50.0f,
        -50.0f,  50.0f, -50.0f,

        -50.0f, -50.0f,  50.0f,
        -50.0f, -50.0f, -50.0f,
        -50.0f,  50.0f, -50.0f,
        -50.0f,  50.0f, -50.0f,
        -50.0f,  50.0f,  50.0f,
        -50.0f, -50.0f,  50.0f,

         50.0f, -50.0f, -50.0f,
         50.0f, -50.0f,  50.0f,
         50.0f,  50.0f,  50.0f,
         50.0f,  50.0f,  50.0f,
         50.0f,  50.0f, -50.0f,
         50.0f, -50.0f, -50.0f,

        -50.0f, -50.0f,  50.0f,
        -50.0f,  50.0f,  50.0f,
         50.0f,  50.0f,  50.0f,
         50.0f,  50.0f,  50.0f,
         50.0f, -50.0f,  50.0f,
        -50.0f, -50.0f,  50.0f,

        -50.0f,  50.0f, -50.0f,
         50.0f,  50.0f, -50.0f,
         50.0f,  50.0f,  50.0f,
         50.0f,  50.0f,  50.0f,
        -50.0f,  50.0f,  50.0f,
        -50.0f,  50.0f, -50.0f,

        -50.0f, -50.0f, -50.0f,
        -50.0f, -50.0f,  50.0f,
         50.0f, -50.0f, -50.0f,
         50.0f, -50.0f, -50.0f,
        -50.0f, -50.0f,  50.0f,
         50.0f, -50.0f,  50.0f
        };

        glGenVertexArrays(1, &skyboxVAO);
        glGenBuffers(1, &skyboxVBO);
        glBindVertexArray(skyboxVAO);
        glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glBindVertexArray(0);
    }
    glBindVertexArray(skyboxVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

