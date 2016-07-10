#pragma once

#include <mitsuba/core/platform.h>
#include <mitsuba/core/fwd.h>
#include <mitsuba/core/warp.h>

#include <Eigen/Core>
#include <nanogui/screen.h>
#include <nanogui/glutil.h>

NAMESPACE_BEGIN(mitsuba)
NAMESPACE_BEGIN(warp)

/**
 * A Nanogui widget to visualize warping functions for different
 * sampling strategies. It also performs a statistical test checking
 * that the warping function matches its PDF and draws the corresponding
 * histograms (observed / expected).
 *
 * Note that it does not implement any UI elements, which are
 * added via inheritance in Python (see \r warp_visualizer.py).
 *
 * This class is decoupled from the UI and implemented in C++
 * so that it can take care of the heavy lifting (warping,
 * binning, draw calls, etc).
 */
class MTS_EXPORT_GUI WarpVisualizationWidget : public nanogui::Screen {

public:
    /// The parameters are passed to the \r nanogui::Screen constructor.
    WarpVisualizationWidget(int width, int height, std::string description);

    virtual ~WarpVisualizationWidget() {
        glDeleteTextures(2, &m_textures[0]);
    }

    /** Run the Chi^2 test for the selected parameters, saves the results
     * and uploads the histograms (observed / expected) to the GPU
     * for rendering.
     */
    bool runTest(double minExpFrequency, double significanceLevel);

    /// Should be called after any UI interaction
    virtual void refresh();

    void setSamplingType(SamplingType s) { m_samplingType = s; }
    void setWarpAdapter(WarpAdapter * wa) { m_warpAdapter = wa; }
    void setPointCount(int n) { m_pointCount = n; }

    bool isDrawingHistogram() { return m_drawHistogram; }
    void setDrawHistogram(bool draw) { m_drawHistogram = draw; }
    bool isDrawingGrid() { return m_drawGrid; }
    void setDrawGrid(bool draw) { m_drawGrid = draw; }

    virtual bool
    mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i & rel,
                     int button, int modifiers) override;

    virtual bool
    mouseButtonEvent(const nanogui::Vector2i &p, int button,
                     bool down, int modifiers) override;

    virtual void drawContents() override;

private:
    void drawHistogram(const nanogui::Vector2i &position,
                       const nanogui::Vector2i &dimensions, GLuint tex);

    void drawGrid(const Eigen::Matrix4f& mvp);

    void initializeVisualizerGUI();

    void framebufferSizeChanged() {
        m_arcball.setSize(mSize);
    }

private:
    std::unique_ptr<nanogui::GLShader> m_pointShader;
    std::unique_ptr<nanogui::GLShader> m_gridShader;
    std::unique_ptr<nanogui::GLShader> m_histogramShader;
    std::unique_ptr<nanogui::GLShader> m_arrowShader;
    GLuint m_textures[2];
    nanogui::Arcball m_arcball;

    SamplingType m_samplingType;
    ref<WarpAdapter> m_warpAdapter;

    bool m_drawHistogram, m_drawGrid;
    size_t m_pointCount, m_lineCount;
    bool m_testResult;
    std::string m_testResultText;
};

NAMESPACE_END(warp)
NAMESPACE_END(mitsuba)
