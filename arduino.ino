#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

bool handleGet = true;
// To stop handling remaining get requests which were sent before game ended
// but did not receive a response yet

const char* ssid = "vnnm_hotspot";  // OH NO
const char* password = "00000000";  // DOXXED

// Pins concerning the ultrasonic sensor
const int trig_pin = 2;
const int echo_pin = 4;

double reading = -1;  // Reading of ultrasonic sensor
double v_sound = 340; // Speed of sound, lazy estimate

String cse_ip = "192.168.0.239"; // OM2M server IP
String cse_port = "8080";        // OM2M server port
String om2m2_server = "http://" + cse_ip + ":" + cse_port + "/~/in-cse/in-name/";
String ae = "GameData";          // Name of application entity

WebServer server(80);

// Creates a content instance which is posted to the container "cnt" with value "val"
// cnf is used just as a place holder
int createCI(String &cnt, String cnf, String& val) {
  HTTPClient http;
  String gourl = om2m2_server + ae + "/" + cnt; // url of the container to be posted to
  Serial.println(gourl);
  http.begin(om2m2_server + ae + "/" + cnt);    // initiate HTTP comm

  // Headers for auth .. Can create separate id and passwd for production
  http.addHeader("X-M2M-Origin", "admin:admin");
  http.addHeader("Content-Type", "application/json;ty=4");

  int code = http.POST("{\"m2m:cin\": {\"cnf\":\"" + cnf + "\",\"con\": \"" + val + "\",\"lbl\": \"\"}}");

  Serial.print("Response Code: ");
  Serial.println(code);
  if (code == -1) {
    Serial.println("Unable To Connect To The Server");
  }
  
  http.end();
}


// Creates a container with name = cntname,
//                          mni = 120, 
// in the application entity "ae"
int createCNT(String& cntname) {
  HTTPClient http;
  http.begin(om2m2_server + ae);

  // headers for auth
  http.addHeader("X-M2M-Origin", "admin:admin");
  http.addHeader("Content-Type", "application/json;ty=3");

  int code = http.POST("{\"m2m:cnt\": {\"rn\": \"" + cntname + "\",\"mni\": 120,\"lbl\": \"\"}}");
  if (code != 201) {
    Serial.println("Unable To Create Container: " + cntname);
    Serial.print("Resp Code: ");
    Serial.println(code);
  }

  http.end();
}

// handles get request to "/"
void servweb(){
  String txt = "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n    <meta charset=\"UTF-8\">\n    <meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\">\n    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n    <script src=\"https://cdn.plot.ly/plotly-latest.min.js\"></script>\n\n    <title>BreakOut</title>\n    <style>\n        body{\n            margin: 0px;\n        }\n        /* #plot{\n            position: fixed;\n            height: 100px;\n            width: 200px;\n            opacity: 50%;\n        } */\n        #end{\n            display: none;\n            position: fixed;\n            height: 80vh;\n            top: 10vh;\n            width: 80vw;\n            left: 10vw;\n            background-color: white;  \n            border: 3px solid black;\n            padding: 20px;\n        }\n        #retry-button{\n            position: relative;\n            left: 38vw;\n            background-color: orange;\n            padding: 20px;\n            border: none;\n            color: white;\n            font-size: large;\n            border-radius: 60px;\n        }\n        #retry-button:hover{\n            cursor: pointer;\n        }\n    </style>\n</head>\n<body>\n    <canvas id=\"game\" width=\"100\" height=\"100\"><h1>some error occured</h1></canvas>\n    <div id = \"end\">\n        <h1 id=\"score\">Score: </h1>\n        <div id=\"plot\"></div>\n        <div id=\"hash\"></div>\n        <button onclick=\"window.location.reload()\" id = \"retry-button\">Retry</button>\n    </div>\n    \n    <script>\n        alert(\'start\')\n        // smoothing algorithm data \n        dt = 0;\n        esp_data = -1;\n        offset = 600;\n        cur_cursor_position = -1;\n        alpha = 0.3;\n        scale = 60;\n        no_in_queue = 1;\n        tolerance = 100;\n        still_tolerance = 20;\n        motions = [];\n        var mainGameLoop;\n        var mainDataLoop;\n        var scoreInc = 200;\n        const canvas = document.getElementById(\'game\');\n        canvas.width = window.innerWidth-10;\n        canvas.height = window.innerHeight-10;\n        /** @type {CanvasRenderingContext2D} */\n        const ctx = canvas.getContext(\'2d\');\n        const MAX_BRICK_WIDTH = canvas.width/10;\n        const BRICK_PADDING  = 10;\n        const MAXVEL = 3;\n        const brickHeight = 35;\n        const paddleHeight = 20;\n        const paddleWidth = 200;\n        const ballRadius = 15;\n        const rowCount = 4;\n        const topPad = 200;\n        const coyote = 0;\n        var gameStaus = true;\n        var ballY = canvas.height - 2 * ballRadius - 50;\n        var ballX = canvas.width/2;\n        var paddleX = canvas.width/2;\n        var prevPaddleX = canvas.width/2;\n        var vx = 0;\n        var vy = -10;\n        var friction = 0.05;\n        var randfact = 0;\n        var score = 0;\n        var maxScore = 0;\n        var noOfBricks = 0;\n\n        var timedata = [];\n        var paddleXdata = [];\n        var ballXdata = [];\n\n        var timedataslow = [];\n        var paddleXdataslow = [];\n        var ballXdataslow = [];\n\n        var time = 0\n        const colors = [\'#fc2617\'/*red*/ ,\'#fca117\' /*orange*/, \'#fcf117\' /*yellow*/, \'#36ff4a\' /*green*/]\n        \n        // document.addEventListener(\"mousemove\", mouseMoveHandler, false);\n        function makeid(length) {\n            var result           = \'\';\n            var characters       = \'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789\';\n            var charactersLength = characters.length;\n            for ( var i = 0; i < length; i++ ) {\n                result += characters.charAt(Math.floor(Math.random() * charactersLength));\n            }\n            return result;\n        }\n\n        function mouseMoveHandler(e) {\n            var relativeX = e.clientX - canvas.offsetLeft;\n            if(relativeX > 0 && relativeX < canvas.width) {\n                paddleX = relativeX;\n            }\n        }\n        function checkPaddle(){\n            if(ballX > paddleX - paddleWidth/2 - coyote && ballX < paddleX + paddleWidth/2 + coyote){\n                if(ballY >= canvas.height - paddleHeight - ballRadius){\n                    // relx = vx - paddleVelocity;\n                    // rely = vy;\n                    vy = -1*Math.abs(vy);\n                    vx += (ballX - paddleX) * friction;\n                }\n            }\n        }\n        function plotVelocity(){\n            // Define Data\n            var data = [{\n                x:timedata,\n                y:paddleXdata,\n                mode:\"lines\"\n            },\n            {\n                x:timedata,\n                y:ballXdata,\n                mode:\"lines\"\n            },\n            ];\n            // Define Layout\n            var layout = {\n                xaxis: {range: [time[0], time[-1]], title: \"Time\"},\n                yaxis: {range: [0, 2000], title: \"Velocity\"},  \n                title: \"Velocity vs. Time\"\n            };\n\n            // Display using Plotly\n            Plotly.newPlot(\"plot\", data, layout);\n        }\n        function checkDeath(){\n\n            if(ballY >= canvas.height - paddleHeight/2 || noOfBricks == 0){\n                clearInterval(mainGameLoop);\n                clearInterval(mainDataLoop);\n\n                var xhr = new XMLHttpRequest();\n\n                // try to stop game 7 times\n                for (let bruhgei = 0; bruhgei < 7; bruhgei++) {\n                    xhr.open(\'POST\', \'http://192.168.0.29/gameend\', true);\n                    xhr.send();\n                }\n\n                hash = makeid(40);\n                console.log(hash);\n\n                // location.reload();\n                gameStaus = false;\n                document.getElementById(\"end\").style.display = \"block\";\n                document.getElementById(\"score\").innerHTML = \"Score: \" + score; \n                document.getElementById(\"hash\").innerHTML = \"ID: \" + hash + \"  <button onclick=\\\"copyHash()\\\">Copy</button>\"; \n                plotVelocity();\n\n                posturl = \'http://192.168.0.29/postdata?\'\n                posturl += `gameid=${hash}&`;\n                posturl += `score=${score}&`;\n                posturl += `paddleXdata=${paddleXdataslow.join(\',\')}&`;\n                posturl += `ballXdata=${ballXdataslow.join(\',\')}&`;\n                posturl += `timedata=${timedataslow.join(\',\')}`;\n\n                console.log(\"LOOK HERE: \" + posturl);\n\n                xhr.open(\'POST\', posturl, true);\n                xhr.send();\n            }\n        }\n        function copyHash(){\n            navigator.clipboard.writeText(hash);\n        }\n        function collisionDetection(){\n            checkPaddle();\n            for(let i = 0; i < rowCount; i++){\n                for(let j = 0; j < rows[i].length; j++){\n                    if(rows[i][j].isVisible != 1)continue;\n                    // if(ballX > rows[i][j].x - coyote && ballX < rows){}\n                    \n                    if(ballY + ballRadius >= rows[i][j].y && ballY - ballRadius  <= rows[i][j].y + rows[i][j].height){\n                        let val1 = ballX - ballRadius - rows[i][j].x;\n                        if(val1 <= rows[i][j].width && val1 >= 0){\n                            vx *= -1;\n                            vy += randfact*(Math.random()-0.5);\n                            rows[i][j].isVisible = 0;\n                        }\n                        if(ballX + ballRadius >= rows[i][j].x &&  ballX + ballRadius < rows[i][j].x + rows[i][j].width){\n                            vx *= -1;\n                            vy += randfact*(Math.random()-0.5);\n                            rows[i][j].isVisible = 0;\n                        }\n                    }\n                    if(ballX + ballRadius >= rows[i][j].x && ballX - ballRadius  <= rows[i][j].x + rows[i][j].width){\n                        let val1 = ballY - ballRadius - rows[i][j].y;\n                        if(ballY - ballRadius - rows[i][j].y <= rows[i][j].height && val1 >= 0){\n                            vy *= -1;\n                            vx += randfact*(Math.random()-0.5);\n                            rows[i][j].isVisible = 0;\n                        }\n                        if(ballY + ballRadius >= rows[i][j].y && ballY + ballRadius <= rows[i][j].y + rows[i][j].height){\n                            vy *= -1;\n                            vx += randfact*(Math.random()-0.5);\n                            rows[i][j].isVisible = 0;\n                        }\n                    }\n                    if(rows[i][j].isVisible == 0){\n                        score += scoreInc;\n                        noOfBricks--;\n                        return;\n                    }\n                }\n            }\n        }\n        function updateBallPosition(){\n            ballX += vx;\n            if(ballX >= canvas.width - ballRadius)vx = -1*Math.abs(vx);\n            if(ballX <= ballRadius) vx = Math.abs(vx);\n            ballY += vy;\n            if(ballY <= ballRadius)vy = Math.abs(vy);\n            if(vx > MAXVEL) vx = MAXVEL;\n            if(vx < -MAXVEL) vx = -MAXVEL;\n            if(vy > MAXVEL) vy = MAXVEL;\n            if(vy < -MAXVEL) vy = -MAXVEL;\n            paddleXdata.push(Math.abs(paddleX));\n            ballXdata.push(ballX);\n            prevPaddleX = paddleX;\n            timedata.push(time)\n            time += 1\n            // if(ballY >= canvas.height - ballRadius)vy *= -1;\n        }\n        // Make background\n        function makeBackground(){\n            ctx.fillStyle = \'#f7f7f7\';\n            ctx.fillRect(0, 0, canvas.width, canvas.height);\n            // console.log(canvas.height, canvas.width);\n        }\n        function createBrick(x, y, w, h){\n            return {x : x, y: y, height: h, width: w, isVisible: 1};\n        }\n        console.log(createBrick(10, 10, 20, 20))\n        \n        function createRow(y){\n            let bricks = []\n            let l = canvas.width;\n            while(l > MAX_BRICK_WIDTH){\n                let w = MAX_BRICK_WIDTH * (Math.random() * 0.7 + 0.3);\n                bricks.push(createBrick(l - w - BRICK_PADDING, y, w, brickHeight));\n                l -= w + BRICK_PADDING;\n                maxScore += scoreInc;\n                noOfBricks++;\n            }\n            bricks.push(createBrick(3, y, l - BRICK_PADDING, brickHeight));\n            maxScore += scoreInc;\n            noOfBricks++;\n            return bricks;\n        }\n        rows = []\n        for(let i = 0; i < rowCount; i++){\n                bricks = createRow(topPad + i * (brickHeight + BRICK_PADDING));\n                rows.push(bricks);\n        }\n        console.log(maxScore);\n        function drawRow(){\n            for(let i = 0; i < rowCount; i++){\n                ctx.fillStyle = colors[i];\n                for(let brick of rows[i]){\n                    if(brick.isVisible == 0)continue;\n                    ctx.fillRect(brick.x, brick.y, brick.width, brick.height);\n                    // console.log(brick);\n                }\n            }\n        }\n        function drawPaddle(){\n            ctx.fillStyle = \"#34b6ba\";\n            ctx.fillRect(paddleX - paddleWidth/2, canvas.height-paddleHeight, paddleWidth, paddleHeight)\n        }\n        function drawBall(){\n            ctx.beginPath();\n            ctx.arc(ballX, ballY, ballRadius, 0, Math.PI*2);\n            ctx.fillStyle = \"#ff0800\";\n            ctx.fill();\n            ctx.closePath();\n        }\n        function drawScore(){\n            ctx.font = \"30px Arial\";\n            ctx.fillStyle = \"#34b6ba\";\n            ctx.fillText(\"Score:\" + score, 10, 50, 100);\n            // ctx.fillText(\"score\", 10, 10, 100);\n        }\n        mainGameLoop = setInterval(()=>{\n            dt++;\n            // Requesting data\n            var xhttp = new XMLHttpRequest();\n            xhttp.onreadystatechange = function() {\n                if (this.readyState == 4 && this.status == 200) {\n                // Typical action to be performed when the document is ready:\n                let resp = xhttp.responseText;\n\n                if (esp_data != 0) {\n                    if (esp_data == -1)\n                        esp_data = Number(resp);\n                    else\n                        esp_data = esp_data * (1 - alpha) + alpha * Number(resp);\n                }\n\n                // console.log(`Raw: ${resp}`);\n\n                motion_value = Math.round(esp_data * scale);\n                // console.log(`Reformed: ${motion_value}`);\n                motions.push(motion_value);\n\n                if (motions.length > no_in_queue) {\n                    new_position = motions.shift();\n                    if (Math.abs(new_position - cur_cursor_position) >= still_tolerance) {\n                        // tween(cur_cursor_position, new_position);\n\n                        console.log((new_position - cur_cursor_position) / dt);\n                        dt = 0;\n\n                        mouseMoveHandler({clientX: Number(new_position) - offset});\n                        cur_cursor_position = new_position;\n                    }\n                }\n\n                // mouseMoveHandler({clientX: Number(resp)*50});\n            }\n            };\n            xhttp.open(\"GET\", \"http://192.168.0.29/data.txt\", true);\n            xhttp.send();\n \n        }, 1000/30);\n\n        mainDataLoop = setInterval(() => {\n            paddleXdataslow.push(Math.abs(paddleX).toFixed(2));\n            ballXdataslow.push(ballX.toFixed(2));\n            timedataslow.push(time)\n        }, 1000/4);\n\n        function draw(){\n            ctx.clearRect(0, 0, canvas.width, canvas.height);\n            drawRow();\n            drawPaddle();\n            drawBall();\n            checkDeath();\n            collisionDetection();\n            updateBallPosition();\n            if(gameStaus)\n                requestAnimationFrame(draw);\n            \n            drawScore();\n        }\n\n        // let renderLoop = setInterval(draw, 1000);\n        scoreInterval = setInterval(() => { scoreInc -= 5; }, 20 * 1000);\n\n        window.onload = function() {\n            draw();\n            let bruh = new XMLHttpRequest();\n\n            bruh.open(\"POST\", \"http://192.168.0.29/gamestart\", true);\n            bruh.send();\n\n        };\n    </script>\n</body>\n</html>";
  server.send(200, "text/html", txt);
}

// Handles data posted to the arduino server from the game
// The data sent is gameID(generated by the client)*
//                  paddle x coordinates
//                  ball x coordinates
//                  times at which data was collected
// *Note: creation of the game ID uses a method which reduces the probability of repeats
// So generating on the client side is fine, although malicious users might take
// advantage of this system but since this is just a harmless game, its alright  

void handlePOST() {
  Serial.println("We got post");
  if (server.hasArg("gameid") == false || server.hasArg("paddleXdata") == false
    || server.hasArg("ballXdata") == false || server.hasArg("timedata") == false
    || server.hasArg("score") == false)
  {
    Serial.println("Invalid Arguments");
    server.send(500, "text/plain", "invalid arguments");
    return;
  }
  // Store the data posted to the server
  String gameid = server.arg("gameid");
  String score = server.arg("score");
  String paddleXdata = server.arg("paddleXdata");
  String ballXdata = server.arg("ballXdata");
  String timedata = server.arg("timedata");
  

  Serial.print("GameId: ");
  Serial.println(gameid);
  Serial.print("score: ");
  Serial.println(score);
  Serial.print("paddleXdata: ");
  Serial.println(paddleXdata);
  Serial.print("ballXdata: ");
  Serial.println(ballXdata);
  Serial.print("timedata: ");
  Serial.println(timedata);

// Make a container in the OM2M service
  createCNT(gameid);
  createCI(gameid, "score", score);
  createCI(gameid, "paddleXdata", paddleXdata);
  createCI(gameid, "ballXdata", ballXdata);
  createCI(gameid, "timedata", timedata);
}

void setup() {
  pinMode(trig_pin, OUTPUT);
  pinMode(echo_pin, INPUT);

  Serial.begin(9600);

  
  // Initialising WiFi connection
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Waiting to connect
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  delay(2000);

// Handle GET requests to "/data.txt"
// Serves the reading of the ultrasonic sensor
  server.on("/data.txt", [](){
    if (handleGet) {
      Serial.println("Got GET");
      server.send(200, "text/plain", String(reading));
    } else return "";
  });

  server.on("/", servweb); // Handles GET request to "/"
  server.on("/postdata", HTTP_POST, handlePOST); // Handles POST request to "/postdata"

  // Optimisation to avoid handling useless GET requests to /data.txt after the game ended
  server.on("/gameend", HTTP_POST, []() { handleGet = false; }); // Stop handling if game ended
  server.on("/gamestart", HTTP_POST, []() { handleGet = true; }); // Start handling if game started
  server.begin();
}

void loop() {
  // Measuring the distance using ultrasonic sensor
  digitalWrite(trig_pin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig_pin, LOW);

  // Measure the time it takes to get back the signal
  long duration = pulseIn(echo_pin, HIGH);

  if (duration == 0) {
    pinMode(echo_pin,OUTPUT);
    digitalWrite(echo_pin,LOW);
    delay(1);
    pinMode(echo_pin,INPUT);
  } else {
    // basic speed * time = distance equation
    reading = ((double) v_sound / 20000) * duration;
  }

  server.handleClient();
  delay(10); // should be fast enough for real time reading
}