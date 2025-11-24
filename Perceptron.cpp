#include "Perceptron.h"
#include <cstdlib>
#include <ctime>
#include <fstream>

Perceptron::Perceptron(int inputSize, double lr)
{
    // seed BEFORE generating random values
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    learningRate = lr;
    bias = (static_cast<double>(std::rand()) / RAND_MAX) - 0.5; // [-0.5, 0.5]

    weights.resize(static_cast<size_t>(inputSize));
    for (int i = 0; i < inputSize; ++i)
        weights[static_cast<size_t>(i)] = (static_cast<double>(std::rand()) / RAND_MAX) - 0.5;
}

int Perceptron::activate(double sum)
{
    return (sum >= 0.0) ? 1 : 0;
}

double Perceptron::score(const std::vector<double>& inputs) const
{
    double s = 0.0;
    for (size_t i = 0; i < weights.size(); ++i)
        s += weights[i] * inputs[i];
    s += bias;
    return s;
}

int Perceptron::predict(const std::vector<double>& inputs)
{
    return activate(score(inputs));
}

void Perceptron::train(const std::vector<double>& inputs, int expectedOutput)
{
    double yHat = score(inputs); // raw output before threshold
    double grad = yHat - expectedOutput; // derivative of loss w.r.t output

    // update weights using gradient
    for (size_t i = 0; i < weights.size(); ++i)
        weights[i] -= learningRate * grad * inputs[i]; // subtract gradient * LR

    // update bias
    bias -= learningRate * grad;
}


bool Perceptron::saveModel(const std::string& filename) const
{
    std::ofstream out(filename);
    if (!out) return false;

    out << weights.size() << ' ' << learningRate << ' ' << bias << '\n';
    for (double w : weights) out << w << ' ';
    out << '\n';
    return true;
}

bool Perceptron::loadModel(const std::string& filename)
{
    std::ifstream in(filename);
    if (!in) return false;

    size_t n = 0;
    in >> n >> learningRate >> bias;
    if (!in) return false;

    weights.assign(n, 0.0);
    for (size_t i = 0; i < n; ++i) in >> weights[i];
    return static_cast<bool>(in);
}
