#ifndef PROGRESSBAR_H
#define PROGRESSBAR_H


class ProgressBar
{
public:
    ProgressBar(unsigned totalAmount, unsigned width=50)
        : width(width), totalAmount(totalAmount), quantized(0) {}

    void begin();
    void update(unsigned amount);
    void end();

private:
    unsigned width;
    unsigned totalAmount;
    unsigned quantized;

    void redraw();
};


class ScopedProgressBar : public ProgressBar
{
public:
    ScopedProgressBar(unsigned totalAmount, unsigned width=50)
        : ProgressBar(totalAmount, width) {
        begin();
    }

    ~ScopedProgressBar() {
        end();
    }
};


#endif // PROGRESSBAR_H
