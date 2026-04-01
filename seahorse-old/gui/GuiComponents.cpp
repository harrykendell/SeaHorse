#pragma once

#include "libs/raygui/src/raygui.h"
#include "seahorse.hpp"
#include <iomanip>
#include <thread>
#include <unistd.h>

// Data
#include "gui/resources/UbuntuMonoBold.hpp"

#define VEC_FROM_RVEC(v) std::vector<double>(v.data(), v.data() + v.size())
#define VEC_VEC_FROM_EIGEN(m)                                         \
    ([](Eigen::MatrixXd z) {                                          \
        std::vector<std::vector<double>> m2;                          \
        for (int i = 0; i < z.cols(); i++) {                          \
            m2.push_back(std::vector<double>(z.data() + i * z.rows(), \
                z.data() + z.rows() * (i + 1)));                      \
        };                                                            \
        return m2;                                                    \
    }(m))

#define alignVector2(vec) (Vector2 { (float)(int)vec.x, (float)(int)vec.y })

struct PerfStats {
public:
    int totalthreads, thr, rss, fps;
    double cpu, ram;
    Color fps_c, thr_c, cpu_c, ram_c;

private:
    std::chrono::system_clock::time_point time;
    char statscommand[1024];
    // Execute command through pipe
    std::string exec(const char* cmd)
    {
        std::array<char, 256> buffer; // holds data - we dont expect that much as we
                                      // only pull rss and pcpu
        std::string result;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
        if (!pipe)
            throw std::runtime_error("popen() failed!");

        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
            result += buffer.data();

        return result;
    }

public:
    // Constructor
    PerfStats()
    {
        time = std::chrono::system_clock::now();
        totalthreads = std::thread::hardware_concurrency();
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
        throw std::runtime_error(
            "Windows performance statistics not supported yet");
#elif __APPLE__
        snprintf(PerfStats::statscommand, 1024,
            "echo $(ps %i -o 'pcpu=,rss=') $(ps %i -M | wc -l)", getpid(),
            getpid());
#else // Linux hopefully
        snprintf(PerfStats::statscommand, 1024,
            "echo $(ps -p %i -o 'pcpu=,rss=') $(ps -p %i -T | wc -l)",
            getpid(), getpid());
#endif
        update(true);
    };

    // Get stats again if we have waited long enough
    int update(bool force = false)
    {
        auto newtime = std::chrono::system_clock::now();

        if (((newtime - time).count() > 0.1e6) || force) { // only update every 0.1s
            sscanf(exec(statscommand).c_str(), "%lf %i %i", &(cpu), &(rss), &(thr));
            fps = GetFPS();

            thr -= 1;
            ram = rss * 1024e-9; // convert 1024 byte units to gigabytes
            time = newtime;

            // Colors based on thresholds
            thr_c = thr > totalthreads ? RED : 2 * thr > totalthreads ? ORANGE
                                                                      : LIME;
            cpu_c = cpu > 90 ? RED : cpu > 50 ? ORANGE
                                              : LIME;
            ram_c = ram > 10 ? RED : ram > 5 ? ORANGE
                                             : LIME;
            fps_c = fps < 15 ? RED : fps < 30 ? ORANGE
                                              : LIME;

            return 1;
        }
        return 0;
    }
};

struct graph; // forward declare
struct graphData {
    // General graph data, specialises to line/heatmap etc

    // Constructor
    graphData(graph* owner, Color c, std::string n, double a)
        : owner(owner)
        , color(c)
        , name(n)
        , alpha(a)
    {
    }
    // Destructor
    virtual ~graphData() = default;
    graph* owner; // callback to redraw the plot
public:
    Color color;
    std::string name;
    double alpha = 1;

    float lims[4] = { 0, 1, 0, 1 }; // {lims[0],lims[1],lims[2],lims[3]}

    virtual void setLims() = 0;
    void setLimsOwner(
        bool force = false); // as it accesses graph's member function draw() we
                             // forward declare and define later
    virtual void draw(Rectangle inside_bounds, double plot_lims[4]) = 0;
};

struct graphLine : graphData {
    std::vector<double> xData;
    std::vector<double> yData;

    double pointRadius = 0;
    double lineWidth = 2;

private:
    bool ignoreLims = false;

public:
    // Constructor
    graphLine(graph* owner, std::vector<double> x, std::vector<double> y,
        std::string n, Color c, double alpha, double lineWidth,
        double pointRadius)
        : graphData(owner, c, n, alpha)
    {
        if (x.size() != 0) {
            xData = x;
        }
        if (y.size() != 0) {
            yData = y;
        }
        if (x.size() != y.size()) {
            S_ERROR("Non-matching sizes in data for graph (", name, "): ", x.size(),
                " != ", y.size());
        }
        graphLine::pointRadius = pointRadius;
        graphLine::lineWidth = lineWidth;

        setLims();
    }

public:
    void ignoreGraphLims()
    {
        ignoreLims = true;
        setLimsOwner(true);
    }
    bool ignoresLims() { return ignoreLims; }
    void draw(Rectangle inside_bounds, double plot_lims[4])
    {
        auto the_lims = std::vector<double>(plot_lims, plot_lims + 4);
        if (ignoreLims) // pretend we are the graph's lims
        {
            the_lims = std::vector<double>(lims, lims + 4);
        }
        auto xFactor = (inside_bounds.width) / (the_lims[1] - the_lims[0]);
        auto yFactor = (inside_bounds.height) / (the_lims[3] - the_lims[2]);
        auto xToPos = [&](double x) {
            return (float)((x - the_lims[0]) * xFactor + inside_bounds.x);
        };
        auto yToPos = [&](double y) {
            return (float)(-(y - the_lims[2]) * yFactor + inside_bounds.y + inside_bounds.height);
        };

        Vector2 start, end;
        if (xData.size() == 0 || yData.size() == 0) {
            return;
        }
        for (size_t i = 0; i < xData.size() - 1; i++) {
            start = { xToPos(xData[i]), yToPos(yData[i]) };
            end = { xToPos(xData[i + 1]), yToPos(yData[i + 1]) };
            if (lineWidth != 0)
                DrawLineEx(start, end, lineWidth, ColorAlpha(color, alpha));
            if (pointRadius != 0)
                DrawCircleV(start, pointRadius, color);
        }
    }

    inline size_t len() { return xData.size(); }
    void updateYData(std::vector<double> y)
    {
        if (y.size() != yData.size()) {
            S_ERROR("Non-matching sizes in data for graph (", name,
                "): ", yData.size(), " != ", y.size());
        }
        yData = y;
        setLims();
        setLimsOwner();
    }
    void updateYData(RVec y) { updateYData(VEC_FROM_RVEC(y)); }
    void updateData(std::vector<double> x, std::vector<double> y)
    {
        if (x.size() != y.size()) {
            S_ERROR("Non-matching sizes in data for graph (", name, "): ", x.size(),
                " != ", y.size());
        }
        // Updates the underlying data and subsequently the limits
        yData = y;
        xData = x;
        setLims();
        setLimsOwner();
    }
    void updateData(RVec x, RVec y)
    {
        updateData(VEC_FROM_RVEC(x), VEC_FROM_RVEC(y));
    }
    void appendPoint(double x, double y)
    {
        xData.push_back(x);
        yData.push_back(y);
        // Update Lims without looping over all data
        if (x < lims[0])
            lims[0] = x;
        if (x > lims[1])
            lims[1] = x;
        if (y < lims[2])
            lims[2] = y;
        if (y > lims[3])
            lims[3] = y;
        setLimsOwner();
    }
    void setLims()
    {
        if (xData.size() == 0 || yData.size() == 0) {
            setLimsOwner();
            return;
        };
        const auto [x0, x1] = std::minmax_element(begin(xData), end(xData));
        const auto [y0, y1] = std::minmax_element(begin(yData), end(yData));
        lims[0] = (*x0);
        lims[1] = (*x1);
        lims[2] = (*y0);
        lims[3] = (*y1);
        setLimsOwner();
    }
};

struct graphHeatmap : graphData {
    // Used for limits
    std::vector<double> xData;
    std::vector<double> yData;

    std::vector<std::vector<double>> zData;
    Color color2;

    // Constructor
    graphHeatmap(graph* owner, std::vector<double> x, std::vector<double> y,
        std::vector<std::vector<double>> z, std::string n, Color c,
        Color c2, double alpha = 1)
        : graphData(owner, c, n, alpha)
        , xData(x)
        , yData(y)
        , zData(z)
        , color2(c2)
    {
    }

public:
    void draw(Rectangle inside_bounds, double plot_lims[4])
    {
        auto xWidth = inside_bounds.width / zData.size();
        auto yWidth = inside_bounds.height / zData[0].size();
        auto xOrigin = (lims[0] - plot_lims[0]) * (inside_bounds.width) / (plot_lims[1] - plot_lims[0]) + inside_bounds.x;
        auto yOrigin = -(lims[3] - plot_lims[2]) * (inside_bounds.height) / (plot_lims[3] - plot_lims[2]) + inside_bounds.y + inside_bounds.height;
        // const auto [zMin,zMax] = std::minmax_element(begin(zData), end(zData));
        // // NB CHECK ZDATA HAS SIZE BEFORE DOING THIS

        if (xData.size() == 0 || yData.size() == 0) {
            return;
        }
        for (size_t x = 0; x < zData.size(); x++) {
            for (size_t y = 0; y < zData[x].size(); y++) {
                Color c = ColorFromHSV(zData[x][y] * 2.55, 0.5, 0.5);
                DrawRectangle(floor(xOrigin + xWidth * x), floor(yOrigin + yWidth * y),
                    ceil(xWidth), ceil(yWidth), c);
            }
        }
    }

    inline size_t len() { return xData.size(); }
    void updateData(std::vector<double> x, std::vector<double> y,
        std::vector<std::vector<double>> z)
    {
        // Updates the underlying data and subsequently the limits
        xData = x;
        yData = y;
        zData = z;
        setLims();
    }
    void updateZData(std::vector<std::vector<double>> z)
    {
        // Updates the underlying data and subsequently the limits
        zData = z;
        setLims();
    }
    void setLims()
    {
        if (xData.size() == 0 || yData.size() == 0) {
            setLimsOwner();
            return;
        };
        const auto [x0, x1] = std::minmax_element(begin(xData), end(xData));
        const auto [y0, y1] = std::minmax_element(begin(yData), end(yData));
        lims[0] = (*x0);
        lims[1] = (*x1);
        lims[2] = (*y0);
        lims[3] = (*y1);
        setLimsOwner();
    }
};

struct graph {
    std::vector<std::shared_ptr<graphLine>> lines;
    Rectangle bounds { 0, 0, 0, 0 };

private:
    std::shared_ptr<graphHeatmap> theheatmap = NULL;

    std::vector<double> xTicks;
    std::vector<std::string> xTickLabels;
    double xOffset = 0;
    std::vector<double> yTicks;
    std::vector<std::string> yTickLabels;
    double yOffset = 0;

    double lims[4] = { 0, 0, 0, 0 };

    int fontSize = 20;
    Font fontTtf = LoadFontFromMemory(
        ".ttf", UbuntuMonoBold,
        (sizeof(UbuntuMonoBold) / sizeof(*UbuntuMonoBold)), fontSize, 0, 0);
    int fontSize2 = 15;
    Font fontTtf2 = LoadFontFromMemory(
        ".ttf", UbuntuMonoBold,
        (sizeof(UbuntuMonoBold) / sizeof(*UbuntuMonoBold)), fontSize2, 0, 0);

    Vector2 padding = { 0, 8 };

public:
    std::string xLabel = "";
    std::string yLabel = "";
    bool legend = true;
    bool autoLims = true;

public:
    // Constructors
    graph()
        : lines(std::vector<std::shared_ptr<graphLine>>())
        , bounds({ 0, 0, -1, -1 })
        , xLabel("")
        , yLabel("")
    {
    } // default constructor
    graph(Rectangle bounds, std::string xLabel, std::string yLabel)
        : lines(std::vector<std::shared_ptr<graphLine>>())
        , xLabel(xLabel)
        , yLabel(yLabel)
    {
        // Avoid pixel issues by rounding to integers
        graph::bounds = { (float)(int)bounds.x, (float)(int)bounds.y,
            (float)(int)bounds.width, (float)(int)bounds.height };
        setLims();
        draw();
    }
    graph(Rectangle bounds)
        : graph(bounds, "", "")
    {
    }

    // Internal functions
private:
    void ReplaceStringInPlace(std::string& subject, const std::string& search,
        const std::string& replace)
    {
        size_t pos = 0;
        while ((pos = subject.find(search, pos)) != std::string::npos) {
            subject.replace(pos, search.length(), replace);
            pos += replace.length();
        }
    }

    double NiceNumber(const double Value, const int Round)
    {
        int Exponent;
        double Fraction;
        double NiceFraction;

        Exponent = (int)floor(log10(Value));
        Fraction = Value / pow(10, (double)Exponent);

        if (Round) {
            if (Fraction < 1.75)
                NiceFraction = 1.0;
            else if (Fraction < 3.0)
                NiceFraction = 2.0;
            else if (Fraction < 5.0)
                NiceFraction = 3.0;
            else if (Fraction < 7.0)
                NiceFraction = 5.0;
            else
                NiceFraction = 10.0;
        } else {
            if (Fraction <= 1.0)
                NiceFraction = 1.0;
            else if (Fraction <= 2.0)
                NiceFraction = 2.0;
            else if (Fraction <= 5.0)
                NiceFraction = 5.0;
            else
                NiceFraction = 10.0;
        }

        return NiceFraction * pow(10, (double)Exponent);
    }

    std::string FormatNumber(double f, double step, double smallestTick)
    {
        std::stringstream ss;
        // If we step small amounts relative to the overall value then we subtract
        // out the min to avoid values like 1.23e3,1.23e3,1.23e3...
        if (step * 100 < std::abs(smallestTick))
            f -= smallestTick;

        ss << std::setprecision(4) << f;
        auto output = ss.str();

        ReplaceStringInPlace(output, "e+0", "e");
        ReplaceStringInPlace(output, "e-0", "e-");
        if (output == "-0")
            output = "0";
        return output;
    }

    Color nextColor()
    {
        static int count = -1;
        static Color cols[] = { PINK, SKYBLUE, GREEN, MAROON, VIOLET,
            DARKBLUE, ORANGE, LIME, MAGENTA };
        count = (count + 1) % (sizeof(cols) / sizeof(cols[0]));
        return cols[count];
    }

    // Plotting functions
public:
    auto plot(std::vector<double> x, std::vector<double> y, std::string name,
        Color color, double alpha = 1, double lineWidth = 2,
        double pointRadius = 0)
    {
        auto newline = std::make_shared<graphLine>(this, x, y, name, color, alpha,
            lineWidth, pointRadius);
        lines.push_back(newline);
        if (autoLims || lines.size() == 1) {
            setLims();
        }
        draw();
        return newline;
    }
    auto plot(std::vector<double> x, std::vector<double> y, std::string n)
    {
        return plot(x, y, n, nextColor());
    }
    auto plot(std::vector<double> x, std::vector<double> y)
    {
        return plot(x, y, "", nextColor());
    }
    auto plot(RVec x, RVec y, std::string n, Color c, double alpha = 1,
        double lineWidth = 2, double pointRadius = 0)
    {
        return plot(VEC_FROM_RVEC(x), VEC_FROM_RVEC(y), n, c, alpha, lineWidth,
            pointRadius);
    }
    auto plot(RVec x, RVec y, std::string n)
    {
        return plot(VEC_FROM_RVEC(x), VEC_FROM_RVEC(y), n, nextColor());
    }
    auto plot(RVec x, RVec y) { return plot(VEC_FROM_RVEC(x), VEC_FROM_RVEC(y)); }

    auto heatmap(std::vector<double> x, std::vector<double> y,
        std::vector<std::vector<double>> z, std::string n, Color c,
        Color c2)
    {
        auto newheatmap = std::make_shared<graphHeatmap>(this, x, y, z, n, c, c2);
        theheatmap = newheatmap;
        setLims();
        draw();
        return newheatmap;
    }
    auto heatmap(std::vector<double> x, std::vector<double> y,
        std::vector<std::vector<double>> z)
    {
        return heatmap(x, y, z, "", BLUE, PURPLE);
    }
    auto heatmap(std::vector<std::vector<double>> z)
    {
        return heatmap(std::vector<double>(0, 1), std::vector<double>(0, 1), z);
    }
    auto heatmap(RVec x, RVec y, Eigen::MatrixXd z, std::string n, Color c,
        Color c2)
    {
        return heatmap(VEC_FROM_RVEC(x), VEC_FROM_RVEC(y), VEC_VEC_FROM_EIGEN(z), n,
            c, c2);
    }
    auto heatmap(RVec x, RVec y, Eigen::MatrixXd z)
    {
        return heatmap(VEC_FROM_RVEC(x), VEC_FROM_RVEC(y), VEC_VEC_FROM_EIGEN(z));
    }
    auto heatmap(Eigen::MatrixXd z) { return heatmap(VEC_VEC_FROM_EIGEN(z)); }

    // Util. functions
    void setLimsFromLine()
    {
        if (autoLims)
            setLims(true);
    }
    void setLims(double xmin, double xmax, double ymin, double ymax)
    {
        xOffset = 0;
        yOffset = 0;
        // Check for and fix issues
        if (xmax < xmin) {
            auto temp = xmax;
            xmax = xmin;
            xmin = temp;
        }
        if (ymax < ymin) {
            auto temp = lims[3];
            lims[3] = lims[2];
            lims[2] = temp;
        }
        if (xmin == xmax) {
            xmin -= 5e-10;
            xmax += 5e-10;
        }
        if (ymin == ymax) {
            ymin -= 5e-10;
            ymax += 5e-10;
        }
        // set lims
        lims[0] = xmin;
        lims[1] = xmax;
        lims[2] = ymin;
        lims[3] = ymax;

        // find nice ticks
        auto xRange = NiceNumber(lims[1] - lims[0], 0);
        auto xTick = NiceNumber(xRange / (8 - 1), 1);
        auto yRange = NiceNumber(lims[3] - lims[2], 0);
        auto yTick = NiceNumber(yRange / (6 - 1), 1);
        // set ticks
        xTicks.clear();
        xTickLabels.clear();
        auto smallestTick = ceil(lims[0] / xTick) * xTick;
        for (auto i = smallestTick; i <= lims[1]; i += xTick) {
            xTicks.push_back(i);
            xTickLabels.push_back(FormatNumber(i, xTick, smallestTick));
        }
        if (xTick * 100 < std::abs(smallestTick)) {
            xOffset = smallestTick;
        }
        yTicks.clear();
        yTickLabels.clear();
        smallestTick = ceil(lims[2] / yTick) * yTick;
        for (auto i = smallestTick; i <= lims[3]; i += yTick) {
            yTicks.push_back(i);
            yTickLabels.push_back(FormatNumber(i, yTick, smallestTick));
        }
        if (yTick * 100 < std::abs(smallestTick)) {
            yOffset = smallestTick;
        }
    }
    void setLims(bool expandOnly = false)
    {
        // Automatically assigns limits to include all data in graph
        bool first = true;
        if (expandOnly) {
            first = false;
        } // We only want to expand auto limits

#define SET_LIMS_XMACRO(line)               \
    if (first || line->lims[0] < lims[0]) { \
        lims[0] = line->lims[0];            \
    };                                      \
    if (first || line->lims[1] > lims[1]) { \
        lims[1] = line->lims[1];            \
    };                                      \
    if (first || line->lims[2] < lims[2]) { \
        lims[2] = line->lims[2];            \
    };                                      \
    if (first || line->lims[3] > lims[3]) { \
        lims[3] = line->lims[3];            \
    };                                      \
    first = false;

        for (auto const& line : lines) {
            if (line->ignoresLims()) {
                continue;
            }
            SET_LIMS_XMACRO(line);
        }
        if (theheatmap != NULL) {
            SET_LIMS_XMACRO(theheatmap);
        }
#undef SET_LIMS_XMACRO

        if (lines.empty()) {
            lims[0] = 0;
            lims[1] = 1;
            lims[2] = 0;
            lims[3] = 1;
        }
        setLims(lims[0], lims[1], lims[2], lims[3]);
    }

    void clearLines()
    {
        lines.clear();
        draw();
    }

    void draw()
    {
        if (autoLims)
            setLims(true);
        auto loc = Vector2 {}; // Used as a buffer for ensuring pixel aligned drawing

        // Account for axis titles/ticks in bounds
        auto xLabelSize = MeasureTextEx(fontTtf, xLabel.c_str(), fontSize, 0);
        auto yLabelSize = MeasureTextEx(fontTtf, yLabel.c_str(), fontSize, 0);
        if (xLabel == "")
            xLabelSize = { 0, 0 };
        if (yLabel == "")
            yLabelSize = { 0, 0 };

        // NB this should really use actual values for ticks/nums but guesses them
        // as fitting within `fontSize`
        auto data_bounds = Rectangle { bounds.x + yLabelSize.y + fontSize, bounds.y,
            bounds.width - yLabelSize.y - fontSize,
            bounds.height - xLabelSize.y - fontSize };

        // Draw the plot data
        auto inside_bounds = Rectangle {
            data_bounds.x + padding.x, data_bounds.y + padding.y,
            data_bounds.width - padding.x * 2, data_bounds.height - padding.y * 2
        };
        auto xFactor = (inside_bounds.width) / (lims[1] - lims[0]);
        auto yFactor = (inside_bounds.height) / (lims[3] - lims[2]);
        auto xToPos = [&](double x) {
            return (float)((x - lims[0]) * xFactor + inside_bounds.x);
        };
        auto yToPos = [&](double y) {
            return (float)(-(y - lims[2]) * yFactor + inside_bounds.y + inside_bounds.height);
        };

        // Clear our area...
        // DrawRectangleRec(bounds, GetColor(GuiGetStyle(DEFAULT,
        // BACKGROUND_COLOR)));
        DrawRectangleRec(data_bounds, WHITE);

        // Draw lines
        BeginScissorMode(data_bounds.x, data_bounds.y, data_bounds.width,
            data_bounds.height);
        for (auto const& line : lines)
            line->draw(inside_bounds, lims);

        // Draw Legend
        if (legend) {
            float padding = 10;
            float maxWidth = 0;
            float totalHeight = 0;
            std::vector<float> textHeights = {};
            std::vector<Color> colors = {};
            std::vector<std::string> names = {};
            for (auto line : lines) {
                if (line->name == "") {
                    continue;
                }
                names.push_back(line->name.c_str());
                colors.push_back(line->color);
                auto size = MeasureTextEx(fontTtf2, names.back().c_str(), fontSize2, 0);
                textHeights.push_back(size.y);
                totalHeight += size.y;
                maxWidth = std::max(maxWidth, size.x);
            }
            if (names.size() != 0) {
                auto legend_rect = Rectangle {
                    data_bounds.x + data_bounds.width - maxWidth - padding * 4,
                    data_bounds.y, maxWidth + padding * 4, totalHeight + padding * 2
                };
                DrawRectangleRounded(legend_rect, 0.1, 10, WHITE);
                DrawRectangleRoundedLines(legend_rect, 0.1, 10, DARKGRAY);
                float heightSoFar = 0;
                for (size_t i = 0; i < names.size(); i++) {
                    auto loc = Vector2 { legend_rect.x + 3 * padding,
                        legend_rect.y + padding + heightSoFar };
                    DrawTextEx(fontTtf2, names[i].c_str(), alignVector2(loc), fontSize2,
                        0, BLACK);
                    DrawLineEx(
                        { legend_rect.x + padding, legend_rect.y + padding + heightSoFar + (float)0.5 * textHeights[i] },
                        { legend_rect.x + padding * 2, legend_rect.y + padding + heightSoFar + (float)0.5 * textHeights[i] },
                        4, colors[i]);
                    heightSoFar += textHeights[i];
                }
            }
        }
        if (GuiButton(Rectangle { data_bounds.x + data_bounds.width - 10,
                          data_bounds.y, 10, 10 },
                NULL)) {
            legend = !legend;
        }
        EndScissorMode();

        // Draw axis labels
        BeginScissorMode(bounds.x, bounds.y, bounds.width, bounds.height);
        loc = Vector2 {
            (float)(int)(data_bounds.x + data_bounds.width / 2 - xLabelSize.x / 2),
            (float)(int)(data_bounds.y + data_bounds.height + xLabelSize.y - 2)
        };
        DrawTextEx(fontTtf, xLabel.c_str(), alignVector2(loc), fontSize, 0, BLACK);
        loc = Vector2 { (float)(int)(data_bounds.x - yLabelSize.y * 2 + 2),
            (float)(int)(data_bounds.y + data_bounds.height / 2 + yLabelSize.x / 2) };
        DrawTextPro(fontTtf, yLabel.c_str(), alignVector2(loc), Vector2 { 0, 0 }, -90,
            fontSize, 0, BLACK);

        // Draw ticks + numbers
        for (size_t i = 0; i < xTicks.size(); i++) {
            auto xtick = xTicks.at(i);
            auto xticklabel = xTickLabels.at(i).c_str();
            DrawLineEx(Vector2 { xToPos(xtick), data_bounds.y + data_bounds.height },
                Vector2 { xToPos(xtick),
                    data_bounds.y + data_bounds.height - fontSize / 3 },
                2, DARKGRAY);

            auto textSize = MeasureTextEx(fontTtf, xticklabel, fontSize2, 0);
            // If we arent fully in the plot area dont show the tick label
            auto loc = Vector2 { xToPos(xtick) - textSize.x / 2,
                data_bounds.y + data_bounds.height + 2 };
            if (loc.x < bounds.x || loc.x + textSize.x > bounds.x + bounds.width) {
                continue;
            }
            DrawTextEx(fontTtf, xticklabel, alignVector2(loc), fontSize2, 0,
                DARKGRAY);
        }
        float lastpos = yToPos(lims[2]); // make sure ticklabels dont overlap
        for (size_t i = 0; i < yTicks.size(); i++) {
            auto ytick = yTicks.at(i);
            auto yticklabel = yTickLabels.at(i).c_str();
            DrawLineEx(Vector2 { data_bounds.x, yToPos(ytick) },
                Vector2 { data_bounds.x + fontSize2 / 3, yToPos(ytick) }, 2,
                DARKGRAY);

            auto textSize = MeasureTextEx(fontTtf, yticklabel, fontSize2, 0);
            // If we aren't fully in the plot area or overlap the last label dont show
            // the label
            auto loc = Vector2 { data_bounds.x - textSize.y - 2,
                yToPos(ytick) + textSize.x / 2 };
            if (loc.y - textSize.x < bounds.y || loc.y > data_bounds.y + data_bounds.height || loc.y > (lastpos - textSize.y / 2)) {
                continue;
            }
            lastpos = loc.y - textSize.x;
            DrawTextPro(fontTtf, yticklabel, alignVector2(loc), Vector2 { 0, 0 }, -90,
                fontSize2, 0, DARKGRAY);
        }

        // Draw axis offsets (used if the range << values)
        if (xOffset != 0) {
            std::stringstream ss;
            ss << "+(" << std::setprecision(4) << xOffset << ")";
            loc = Vector2 { xToPos(lims[1]) - MeasureTextEx(fontTtf2, ss.str().c_str(), fontSize2, 0).x,
                data_bounds.y + data_bounds.height + 2 + fontSize2 };
            DrawTextEx(fontTtf2, ss.str().c_str(), alignVector2(loc), fontSize2, 0,
                DARKGRAY);
        }
        if (yOffset != 0) {
            std::stringstream ss;
            ss << "+(" << std::setprecision(4) << yOffset << ")";
            auto textSize = MeasureTextEx(fontTtf2, ss.str().c_str(), fontSize2, 0);
            loc = Vector2 { data_bounds.x - 2 * textSize.y - 2,
                yToPos(lims[3]) + textSize.x };
            DrawTextPro(fontTtf2, ss.str().c_str(), alignVector2(loc), Vector2 { 0, 0 },
                -90, fontSize2, 0, DARKGRAY);
        }

        // Draw graph outline
        auto outline_bounds = Rectangle { data_bounds.x, data_bounds.y,
            data_bounds.width, data_bounds.height };
        DrawRectangleLinesEx(outline_bounds, 1, BLACK);
        EndScissorMode();
    }
};

// Delayed definition when graph->draw() is complete
void graphData::setLimsOwner(bool force)
{
    // If we are calling this the graphData has changed somehow so we need to
    // update the graph's limits
    if (force) {
        owner->setLims();
    } else {
        owner->setLimsFromLine();
    }
}
