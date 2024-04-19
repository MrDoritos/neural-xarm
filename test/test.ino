#include <Arduino.h>
#include <math.h>

// Define the number of input and output contexts
#define NUM_INPUT_CONTEXTS 3
#define NUM_OUTPUT_CONTEXTS 2

// Define the number of neurons in each layer
#define NUM_INPUT_NEURONS NUM_INPUT_CONTEXTS
#define NUM_HIDDEN_NEURONS 5
#define NUM_OUTPUT_NEURONS NUM_OUTPUT_CONTEXTS

// Define the learning rate and number of training iterations
#define LEARNING_RATE 0.1
#define NUM_ITERATIONS 1000

// Define the weights and biases for the neural network
float hiddenWeights[NUM_INPUT_NEURONS][NUM_HIDDEN_NEURONS];
float hiddenBiases[NUM_HIDDEN_NEURONS];
float outputWeights[NUM_HIDDEN_NEURONS][NUM_OUTPUT_NEURONS];
float outputBiases[NUM_OUTPUT_NEURONS];

// Activation function (sigmoid)
float sigmoid(float x) {
  return 1.0 / (1.0 + exp(-x));
}

// Forward propagation
void forwardPropagation(float* inputs, float* hiddenOutputs, float* outputs) {
  // Calculate hidden layer outputs
  for (int i = 0; i < NUM_HIDDEN_NEURONS; i++) {
    float sum = hiddenBiases[i];
    for (int j = 0; j < NUM_INPUT_NEURONS; j++) {
      sum += inputs[j] * hiddenWeights[j][i];
    }
    hiddenOutputs[i] = sigmoid(sum);
  }

  // Calculate output layer outputs
  for (int i = 0; i < NUM_OUTPUT_NEURONS; i++) {
    float sum = outputBiases[i];
    for (int j = 0; j < NUM_HIDDEN_NEURONS; j++) {
      sum += hiddenOutputs[j] * outputWeights[j][i];
    }
    outputs[i] = sigmoid(sum);
  }
}

// Backpropagation
void backpropagation(float* inputs, float* targets, float* hiddenOutputs, float* outputs) {
  float outputDeltas[NUM_OUTPUT_NEURONS];
  float hiddenDeltas[NUM_HIDDEN_NEURONS];

  // Calculate output layer deltas
  for (int i = 0; i < NUM_OUTPUT_NEURONS; i++) {
    outputDeltas[i] = (targets[i] - outputs[i]) * outputs[i] * (1 - outputs[i]);
  }

  // Calculate hidden layer deltas
  for (int i = 0; i < NUM_HIDDEN_NEURONS; i++) {
    float sum = 0;
    for (int j = 0; j < NUM_OUTPUT_NEURONS; j++) {
      sum += outputDeltas[j] * outputWeights[i][j];
    }
    hiddenDeltas[i] = sum * hiddenOutputs[i] * (1 - hiddenOutputs[i]);
  }

  // Update output layer weights and biases
  for (int i = 0; i < NUM_HIDDEN_NEURONS; i++) {
    for (int j = 0; j < NUM_OUTPUT_NEURONS; j++) {
      outputWeights[i][j] += LEARNING_RATE * outputDeltas[j] * hiddenOutputs[i];
    }
  }
  for (int i = 0; i < NUM_OUTPUT_NEURONS; i++) {
    outputBiases[i] += LEARNING_RATE * outputDeltas[i];
  }

  // Update hidden layer weights and biases
  for (int i = 0; i < NUM_INPUT_NEURONS; i++) {
    for (int j = 0; j < NUM_HIDDEN_NEURONS; j++) {
      hiddenWeights[i][j] += LEARNING_RATE * hiddenDeltas[j] * inputs[i];
    }
  }
  for (int i = 0; i < NUM_HIDDEN_NEURONS; i++) {
    hiddenBiases[i] += LEARNING_RATE * hiddenDeltas[i];
  }
}

// Train the neural network
void trainNetwork(float** trainingData, int numSamples) {
  float hiddenOutputs[NUM_HIDDEN_NEURONS];
  float outputs[NUM_OUTPUT_NEURONS];

  for (int i = 0; i < NUM_ITERATIONS; i++) {
    for (int j = 0; j < numSamples; j++) {
      forwardPropagation(trainingData[j], hiddenOutputs, outputs);
      backpropagation(trainingData[j], trainingData[j] + NUM_INPUT_CONTEXTS, hiddenOutputs, outputs);
    }
  }
}

// Initialize the weights and biases randomly
void initializeWeights() {
  for (int i = 0; i < NUM_INPUT_NEURONS; i++) {
    for (int j = 0; j < NUM_HIDDEN_NEURONS; j++) {
      hiddenWeights[i][j] = (float)random(-1000, 1000) / 1000.0;
    }
  }
  for (int i = 0; i < NUM_HIDDEN_NEURONS; i++) {
    hiddenBiases[i] = (float)random(-1000, 1000) / 1000.0;
    for (int j = 0; j < NUM_OUTPUT_NEURONS; j++) {
      outputWeights[i][j] = (float)random(-1000, 1000) / 1000.0;
    }
  }
  for (int i = 0; i < NUM_OUTPUT_NEURONS; i++) {
    outputBiases[i] = (float)random(-1000, 1000) / 1000.0;
  }
}

void setup() {
  Serial.begin(9600);
  randomSeed(analogRead(0));
  initializeWeights();
}

void loop() {
  if (Serial.available()) {
    // Read input contexts from Serial
    float inputs[NUM_INPUT_CONTEXTS];
    for (int i = 0; i < NUM_INPUT_CONTEXTS; i++) {
      inputs[i] = Serial.parseFloat();
    }

    // Read target outputs from Serial
    float targets[NUM_OUTPUT_CONTEXTS];
    for (int i = 0; i < NUM_OUTPUT_CONTEXTS; i++) {
      targets[i] = Serial.parseFloat();
    }

    // Create training data
    float* trainingData[1];
    trainingData[0] = new float[NUM_INPUT_CONTEXTS + NUM_OUTPUT_CONTEXTS];
    for (int i = 0; i < NUM_INPUT_CONTEXTS; i++) {
      trainingData[0][i] = inputs[i];
    }
    for (int i = 0; i < NUM_OUTPUT_CONTEXTS; i++) {
      trainingData[0][NUM_INPUT_CONTEXTS + i] = targets[i];
    }

    // Train the network
    trainNetwork(trainingData, 1);

    // Forward propagate the inputs
    float hiddenOutputs[NUM_HIDDEN_NEURONS];
    float outputs[NUM_OUTPUT_NEURONS];
    forwardPropagation(inputs, hiddenOutputs, outputs);

    // Print the outputs
    for (int i = 0; i < NUM_OUTPUT_NEURONS; i++) {
      Serial.print(outputs[i]);
      Serial.print(" ");
    }
    Serial.println();

    // Free the dynamically allocated memory
    delete[] trainingData[0];
  }
}
