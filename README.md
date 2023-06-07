# YYAT IoT Project
### Project Idea
Contactless Breakout
### Abstract
Our project, Contactless Breakout, aims to create a contactless input using hand movements for playing the famous Atari Breakout game. Our motivation is to make the game more fun by providing a mode of input that is more immersive than using a mouse or keyboard, increasing the quality of the player's gaming experience. Our project will use ultrasonic sensors to detect the hand movement of the user. This will be used to interact with the game.
### Components Required
* 1 ESP32
* 4 Ultrasonic Sensors (HC-SR04)
* 2 PIR Sensors
* Connecting Wires
* LED(s)
* Push Button
### Parameters measured
* **Distance** will be measured with the help of HC-SR04 sensors. This distance will be the main input for controlling the platform in the game. We will have ultrasonic sensors on the sides and the input area between them. The distance measured by the sensor will be scaled down to move the platform in the game.
* **Speed** will be measured at regular intervals of time by measuring the change in distance in that time interval, small enough that the direction change will not affect it much. This data will be used to analyse the game later.
* **High Score** will be a measure of how well a player performs in the game. A typical measurement is necessary for any game, and can be stored to compare the performances of two players.
### Analysis
We will analyse the high scores of the players and use them to calculate the average score that a player scores. This can be used to judge how good a performance of the player who is playing at that time was, if it was below average, average, or above average. We will use the speed data measured to draw a graph depicting the speed of hand vs time as the game progresses. This data can be used to analyse how the duration of the game affects people's input to the game.

