//
//  GameViewController.m
//  imguiex
//
#import "GameViewController.h"
#import <OpenGLES/ES2/glext.h>

#import "imgui_impl_ios.h"
#import "debug_hud.h"

#define BUFFER_OFFSET(i)        ((char *)NULL + (i))
#define SERVERNAME_KEY          @"ServerName"
#define SERVERNAME_ALERT_TAG    10

// Uniform index.
enum
{
    UNIFORM_MODELVIEWPROJECTION_MATRIX,
    UNIFORM_NORMAL_MATRIX,
    UNIFORM_DIFFUSE_COLOR,
    UNIFORM_COUNT_
};
static GLint uniforms[UNIFORM_COUNT_];

// Attribute index.
enum
{
    ATTRIB_VERTEX,
    ATTRIB_NORMAL,
    ATTRIB_COUNT_
};

static const GLfloat gCubeVertexData[216] = 
{
    // Data layout for each line below is:
    // pos x/y/z,           normal x/y/z,
    0.5f, -0.5f, -0.5f,     1.0f, 0.0f, 0.0f,
    0.5f,  0.5f, -0.5f,     1.0f, 0.0f, 0.0f,
    0.5f, -0.5f,  0.5f,     1.0f, 0.0f, 0.0f,
    0.5f, -0.5f,  0.5f,     1.0f, 0.0f, 0.0f,
    0.5f,  0.5f, -0.5f,     1.0f, 0.0f, 0.0f,
    0.5f,  0.5f,  0.5f,     1.0f, 0.0f, 0.0f,
    
     0.5f, 0.5f, -0.5f,     0.0f, 1.0f, 0.0f,
    -0.5f, 0.5f, -0.5f,     0.0f, 1.0f, 0.0f,
     0.5f, 0.5f,  0.5f,     0.0f, 1.0f, 0.0f,
     0.5f, 0.5f,  0.5f,     0.0f, 1.0f, 0.0f,
    -0.5f, 0.5f, -0.5f,     0.0f, 1.0f, 0.0f,
    -0.5f, 0.5f,  0.5f,     0.0f, 1.0f, 0.0f,
    
    -0.5f,  0.5f, -0.5f,    -1.0f, 0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,    -1.0f, 0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,    -1.0f, 0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,    -1.0f, 0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,    -1.0f, 0.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,    -1.0f, 0.0f, 0.0f,
    
    -0.5f, -0.5f, -0.5f,    0.0f, -1.0f, 0.0f,
     0.5f, -0.5f, -0.5f,    0.0f, -1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,    0.0f, -1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,    0.0f, -1.0f, 0.0f,
     0.5f, -0.5f, -0.5f,    0.0f, -1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,    0.0f, -1.0f, 0.0f,
    
     0.5f,  0.5f, 0.5f,     0.0f, 0.0f, 1.0f,
    -0.5f,  0.5f, 0.5f,     0.0f, 0.0f, 1.0f,
     0.5f, -0.5f, 0.5f,     0.0f, 0.0f, 1.0f,
     0.5f, -0.5f, 0.5f,     0.0f, 0.0f, 1.0f,
    -0.5f,  0.5f, 0.5f,     0.0f, 0.0f, 1.0f,
    -0.5f, -0.5f, 0.5f,     0.0f, 0.0f, 1.0f,
    
     0.5f, -0.5f, -0.5f,    0.0f, 0.0f, -1.0f,
    -0.5f, -0.5f, -0.5f,    0.0f, 0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,    0.0f, 0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,    0.0f, 0.0f, -1.0f,
    -0.5f, -0.5f, -0.5f,    0.0f, 0.0f, -1.0f,
    -0.5f,  0.5f, -0.5f,    0.0f, 0.0f, -1.0f
};

@interface GameViewController () <UIAlertViewDelegate>
{
    GLuint      _program;
    GLKMatrix4  _modelViewProjectionMatrix;
    GLKMatrix3  _normalMatrix;
    float       _rotation;
    GLuint      _vertexArray;
    GLuint      _vertexBuffer;
    DebugHUD    _hud;
}
@property (strong, nonatomic) EAGLContext*      context;
@property (strong, nonatomic) GLKBaseEffect*    effect;
@property (strong, nonatomic) ImGuiHelper*      imgui;
@property (weak, nonatomic) IBOutlet UIButton*  btnServername;
@property (strong, nonatomic) NSString*         serverName;

- (IBAction)onServernameTapped:(id)sender;

- (void)setupGL;
- (void)tearDownGL;

- (BOOL)loadShaders;
- (BOOL)compileShader:(GLuint*)shader type:(GLenum)type file:(NSString*)file;
- (BOOL)linkProgram:(GLuint)prog;
- (BOOL)validateProgram:(GLuint)prog;
@end

@implementation GameViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];

    if (!self.context)
        NSLog(@"Failed to create ES context");
    
    GLKView *view = (GLKView *)self.view;
    view.context = self.context;
    view.drawableDepthFormat = GLKViewDrawableDepthFormat24;
    
    [self.btnServername setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    
    [self setupGL];
    
    NSUserDefaults *userDefaults = [NSUserDefaults standardUserDefaults];
    self.serverName = [userDefaults objectForKey: SERVERNAME_KEY ];
    self.imgui = [[ImGuiHelper alloc] initWithView:self.view ];
    if (self.serverName)
    {
        [self.btnServername setTitle:self.serverName forState:UIControlStateNormal];
        [self.imgui connectServer: self.serverName ];
    }
    
    DebugHUD_InitDefaults(&_hud);
}

- (void)dealloc
{    
    [self tearDownGL];
    
    if ([EAGLContext currentContext] == self.context)
        [EAGLContext setCurrentContext:nil];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];

    if ([self isViewLoaded] && ([[self view] window] == nil)) 
    {
        self.view = nil;
        
        [self tearDownGL];
        
        if ([EAGLContext currentContext] == self.context)
            [EAGLContext setCurrentContext:nil];
        self.context = nil;
    }

    // Dispose of any resources that can be recreated.
}


- (BOOL)prefersStatusBarHidden 
{
    return YES;
}

- (IBAction)onServernameTapped:(id)sender
{
    UIAlertView * alert = [[UIAlertView alloc] initWithTitle:@"Set Server" message:@"Enter server name or IP for uSynergy" delegate:self cancelButtonTitle:@"OK" otherButtonTitles:@"Cancel", nil ];
    alert.alertViewStyle = UIAlertViewStylePlainTextInput;
    alert.tag = SERVERNAME_ALERT_TAG; // cheezy way to tell which alert view we're responding to
    [alert show];
}

- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
    if ((buttonIndex==0) && (alertView.tag == SERVERNAME_ALERT_TAG))
    {
        // This is really janky. I usually just hardcode the servername since I'm building it anyway.
        // If you want to properly handle updating the server, you'll want to tear down and recreate
        // the usynergy stuff in connectServer
        BOOL serverNameWasSet = self.serverName.length > 0;
        NSString *serverName = [[alertView textFieldAtIndex:0] text];

        if ([serverName length] > 0) 
        {
            self.serverName = serverName;
            NSUserDefaults *userDefaults = [NSUserDefaults standardUserDefaults];
            [userDefaults setObject:serverName forKey:SERVERNAME_KEY ];
            [userDefaults synchronize];
            
            [self.btnServername setTitle:self.serverName forState:UIControlStateNormal];
            
            // If we hadn't previously connected, try now
            if (!serverNameWasSet) 
            {
                [self.imgui connectServer:self.serverName];
            }
            else
            {
                UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Servername Updated"
                                                                message:@"Restart the app to connect the server"
                                                               delegate:nil cancelButtonTitle:@"OK" otherButtonTitles: nil];
                [alert show];
            }
        }
    }
}

- (void)setupGL
{
    [EAGLContext setCurrentContext:self.context];
    
    [self loadShaders];
    
    self.effect = [[GLKBaseEffect alloc] init];
    self.effect.light0.enabled = GL_TRUE;
    self.effect.light0.diffuseColor = GLKVector4Make(1.0f, 0.4f, 0.4f, 1.0f);
    
    glEnable(GL_DEPTH_TEST);
    
    glGenVertexArraysOES(1, &_vertexArray);
    glBindVertexArrayOES(_vertexArray);
    
    glGenBuffers(1, &_vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, _vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(gCubeVertexData), gCubeVertexData, GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(GLKVertexAttribPosition);
    glVertexAttribPointer(GLKVertexAttribPosition, 3, GL_FLOAT, GL_FALSE, 24, BUFFER_OFFSET(0));
    glEnableVertexAttribArray(GLKVertexAttribNormal);
    glVertexAttribPointer(GLKVertexAttribNormal, 3, GL_FLOAT, GL_FALSE, 24, BUFFER_OFFSET(12));
    
    glBindVertexArrayOES(0);
}

- (void)tearDownGL
{
    [EAGLContext setCurrentContext:self.context];
    
    glDeleteBuffers(1, &_vertexBuffer);
    glDeleteVertexArraysOES(1, &_vertexArray);
    
    self.effect = nil;
    
    if (_program) 
    {
        glDeleteProgram(_program);
        _program = 0;
    }
}

#pragma mark - GLKView and GLKViewController delegate methods

- (void)update
{
    float aspect = fabs(self.view.bounds.size.width / self.view.bounds.size.height);
    GLKMatrix4 projectionMatrix = GLKMatrix4MakePerspective(GLKMathDegreesToRadians(65.0f), aspect, 0.1f, 100.0f);
    
    self.effect.transform.projectionMatrix = projectionMatrix;
    
    GLKMatrix4 baseModelViewMatrix = GLKMatrix4MakeTranslation(0.0f, 0.0f, -4.0f);
    baseModelViewMatrix = GLKMatrix4Rotate(baseModelViewMatrix, _rotation, 0.0f, 1.0f, 0.0f);
    
    // Compute the model view matrix for the object rendered with GLKit
    GLKMatrix4 modelViewMatrix = GLKMatrix4MakeTranslation(0.0f, 0.0f, -1.5f);
    modelViewMatrix = GLKMatrix4Rotate(modelViewMatrix, _rotation, 1.0f, 1.0f, 1.0f);
    modelViewMatrix = GLKMatrix4Multiply(baseModelViewMatrix, modelViewMatrix);
    
    self.effect.transform.modelviewMatrix = modelViewMatrix;
    
    // Compute the model view matrix for the object rendered with ES2
    modelViewMatrix = GLKMatrix4MakeTranslation(0.0f, 0.0f, 1.5f);
    modelViewMatrix = GLKMatrix4Rotate(modelViewMatrix, _rotation, 1.0f, 1.0f, 1.0f);
    modelViewMatrix = GLKMatrix4Multiply(baseModelViewMatrix, modelViewMatrix);
    
    _normalMatrix = GLKMatrix3InvertAndTranspose(GLKMatrix4GetMatrix3(modelViewMatrix), NULL);
    _modelViewProjectionMatrix = GLKMatrix4Multiply(projectionMatrix, modelViewMatrix);
    _rotation += self.timeSinceLastUpdate * (_hud.rotation_speed * (M_PI / 180.0));
}


- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
	// Start the dear imgui frame
    [self.imgui newFrame];
    
    // Create some UI elements
    DebugHUD_DoInterface( &_hud );

	// Render
    glClearColor(0.65f, 0.65f, 0.65f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   
    glBindVertexArrayOES(_vertexArray);
    
    // Render the object with GLKit
    [self.effect prepareToDraw];
    glDrawArrays(GL_TRIANGLES, 0, 36);
    
    // Render the object again with ES2
    glUseProgram(_program);
    glUniformMatrix4fv(uniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX], 1, 0, _modelViewProjectionMatrix.m);
    glUniformMatrix3fv(uniforms[UNIFORM_NORMAL_MATRIX], 1, 0, _normalMatrix.m);
    glUniform3f(uniforms[UNIFORM_DIFFUSE_COLOR], _hud.cubeColor1[0], _hud.cubeColor1[1], _hud.cubeColor1[2] );
    glDrawArrays(GL_TRIANGLES, 0, 36);
        
    self.effect.light0.diffuseColor = GLKVector4Make( _hud.cubeColor2[0], _hud.cubeColor2[1], _hud.cubeColor2[2], 1.0f);
    
    // Render dear imgui as the last thing in the frame if possible
    [self.imgui render];
}

#pragma mark -  OpenGL ES 2 shader compilation

- (BOOL)loadShaders
{
    GLuint vertShader, fragShader;
    NSString *vertShaderPathname, *fragShaderPathname;
    
    // Create shader program.
    _program = glCreateProgram();
    
    // Create and compile vertex shader.
    vertShaderPathname = [[NSBundle mainBundle] pathForResource:@"Shader" ofType:@"vsh"];
    if (![self compileShader:&vertShader type:GL_VERTEX_SHADER file:vertShaderPathname]) {
        NSLog(@"Failed to compile vertex shader");
        return NO;
    }
    
    // Create and compile fragment shader.
    fragShaderPathname = [[NSBundle mainBundle] pathForResource:@"Shader" ofType:@"fsh"];
    if (![self compileShader:&fragShader type:GL_FRAGMENT_SHADER file:fragShaderPathname]) {
        NSLog(@"Failed to compile fragment shader");
        return NO;
    }
    
    // Attach vertex shader to program.
    glAttachShader(_program, vertShader);
    
    // Attach fragment shader to program.
    glAttachShader(_program, fragShader);
    
    // Bind attribute locations.
    // This needs to be done prior to linking.
    glBindAttribLocation(_program, GLKVertexAttribPosition, "position");
    glBindAttribLocation(_program, GLKVertexAttribNormal, "normal");
    
    // Link program.
    if (![self linkProgram:_program]) 
    {
        NSLog(@"Failed to link program: %d", _program);
        
        if (vertShader) {
            glDeleteShader(vertShader);
            vertShader = 0;
        }
        if (fragShader) {
            glDeleteShader(fragShader);
            fragShader = 0;
        }
        if (_program) {
            glDeleteProgram(_program);
            _program = 0;
        }
        
        return NO;
    }
    
    // Get uniform locations.
    uniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX] = glGetUniformLocation(_program, "modelViewProjectionMatrix");
    uniforms[UNIFORM_NORMAL_MATRIX] = glGetUniformLocation(_program, "normalMatrix");
    uniforms[UNIFORM_DIFFUSE_COLOR] = glGetUniformLocation(_program, "diffuseColor");
    
    // Release vertex and fragment shaders.
    if (vertShader) 
    {
        glDetachShader(_program, vertShader);
        glDeleteShader(vertShader);
    }
    if (fragShader) 
    {
        glDetachShader(_program, fragShader);
        glDeleteShader(fragShader);
    }
    
    return YES;
}

- (BOOL)compileShader:(GLuint *)shader type:(GLenum)type file:(NSString *)file
{
    GLint status;
    const GLchar *source;
    
    source = (GLchar *)[[NSString stringWithContentsOfFile:file encoding:NSUTF8StringEncoding error:nil] UTF8String];
    if (!source) 
    {
        NSLog(@"Failed to load vertex shader");
        return NO;
    }
    
    *shader = glCreateShader(type);
    glShaderSource(*shader, 1, &source, NULL);
    glCompileShader(*shader);
    
#if defined(DEBUG)
    GLint logLength;
    glGetShaderiv(*shader, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0) {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetShaderInfoLog(*shader, logLength, &logLength, log);
        NSLog(@"Shader compile log:\n%s", log);
        free(log);
    }
#endif
    
    glGetShaderiv(*shader, GL_COMPILE_STATUS, &status);
    if (status == 0) 
    {
        glDeleteShader(*shader);
        return NO;
    }
    
    return YES;
}

- (BOOL)linkProgram:(GLuint)prog
{
    GLint status;
    glLinkProgram(prog);
    
#if defined(DEBUG)
    GLint logLength;
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0) 
    {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetProgramInfoLog(prog, logLength, &logLength, log);
        NSLog(@"Program link log:\n%s", log);
        free(log);
    }
#endif
    
    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if (status == 0)
        return NO;
    return YES;
}

- (BOOL)validateProgram:(GLuint)prog
{
    GLint logLength, status;
    
    glValidateProgram(prog);
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0) {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetProgramInfoLog(prog, logLength, &logLength, log);
        NSLog(@"Program validate log:\n%s", log);
        free(log);
    }
    
    glGetProgramiv(prog, GL_VALIDATE_STATUS, &status);
    if (status == 0)
        return NO;
    return YES;
}

@end
