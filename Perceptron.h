#pragma once
#include <vector>
#include <string>
#include <iostream>

class Perceptron
{
private:
    std::vector<double> weights;   // weights for each input
    double learningRate;           // step size for weight updates
    double bias;                   // bias term

public:
    // Constructor: number of inputs and learning rate
    Perceptron(int inputSize, double lr = 0.1);

    // Activation function (step function)
    int activate(double sum);

    // Feedforward: compute output for given inputs
    int predict(const std::vector<double>& inputs);

    // Train on a single sample
    void train(const std::vector<double>& inputs, int expectedOutput);

    // (Optional) raw score (weighted sum + bias), useful for debugging
    double score(const std::vector<double>& inputs) const;

    // Accessors
    std::vector<double> getWeights() const { return weights; }
    double getBias() const { return bias; }

    // Persist model
    bool saveModel(const std::string& filename) const;
    bool loadModel(const std::string& filename);
};