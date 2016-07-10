import gc
from collections import OrderedDict
import math
import mitsuba
from mitsuba import warp, BoundingBox3f
from mitsuba.warp import WarpType, SamplingType, \
                         WarpAdapter, PlaneWarpAdapter, IdentityWarpAdapter, SphereWarpAdapter
from mitsuba.warp import WarpVisualizationWidget

import nanogui
from nanogui import Color, Screen, Window, Widget, GroupLayout, BoxLayout, \
                    Vector2i, Label, Button, TextBox, CheckBox, MessageDialog, \
                    ComboBox, Slider, Alignment, Orientation
from nanogui import glfw, entypo

class WarpVisualizer(WarpVisualizationWidget):
    """Nanogui app allowing to visualize and test warping functions.
    Note that this file is mostly responsible for implementing the GUI, whereas
    heavy-lifting is performed by the C++ WarpVisualizationWidget."""

    # Default values for UI controls
    pointCountDefaultValue = 7.0 / 15.0
    warpParameterDefaultValue = 0.5
    angleDefaultValue = 0.5

    # Default values for statistical test
    minExpFrequency = 5.0
    significanceLevel = 0.01

    def __init__(self):
        super(WarpVisualizer, self).__init__(800, 600, "Warp visualizer")

        # TODO: refactor (this could be useful for the tests as well)
        class WarpFactory:
            def __init__(self, adapter, name, f, pdf, arguments = [], bbox = None):
                self.adapter = adapter
                self.name = name
                self.f = f
                self.pdf = pdf
                self.arguments = arguments
                self.bbox = bbox

            def bind(self, args):
                f = lambda s: self.f(s, **args)
                pdf = lambda v: self.pdf(v, **args)

                if self.bbox:
                    return self.adapter(self.name, f, pdf, self.arguments, self.bbox)
                return self.adapter(self.name, f, pdf, self.arguments)

        class IdentityWarpFactory:
            def __init__(self):
                self.name = "Identity"
                self.arguments = []

            def bind(self, args):
                return IdentityWarpAdapter()

        def warpWithUnitWeight(f):
            return lambda p, *args, **kwargs: (f(p, *args, **kwargs), 1.0)

        w = OrderedDict()
        w[WarpType.NoWarp] = IdentityWarpFactory()
        w[WarpType.UniformDisk] = WarpFactory(PlaneWarpAdapter,
            "Square to uniform disk",
            warpWithUnitWeight(warp.squareToUniformDisk),
            warp.squareToUniformDiskPdf)
        w[WarpType.UniformDiskConcentric] = WarpFactory(PlaneWarpAdapter,
            "Square to uniform disk concentric",
            warpWithUnitWeight(warp.squareToUniformDiskConcentric),
            warp.squareToUniformDiskConcentricPdf)
        w[WarpType.UniformTriangle] = WarpFactory(PlaneWarpAdapter,
            "Square to uniform triangle",
            warpWithUnitWeight(warp.squareToUniformTriangle),
            warp.squareToUniformTrianglePdf,
            bbox = WarpAdapter.kUnitSquareBoundingBox)
        # TODO: manage the case of infinite support (need inverse mapping?)
        w[warp.StandardNormal] = WarpFactory(PlaneWarpAdapter,
            "Square to 2D gaussian",
            warpWithUnitWeight(warp.squareToStdNormal),
            warp.squareToStdNormalPdf,
            bbox = BoundingBox3f([-5, -5, -5], [5, 5, 5]))
        w[warp.UniformTent] = WarpFactory(PlaneWarpAdapter,
            "Square to tent",
            warpWithUnitWeight(warp.squareToTent),
            warp.squareToTentPdf)

        # 2D -> 3D warps
        w[WarpType.UniformSphere] = WarpFactory(SphereWarpAdapter,
            "Square to uniform sphere",
            warpWithUnitWeight(warp.squareToUniformSphere),
            warp.squareToUniformSpherePdf)
        w[warp.UniformHemisphere] = WarpFactory(SphereWarpAdapter,
            "Square to uniform hemisphere",
            warpWithUnitWeight(warp.squareToUniformHemisphere),
            warp.squareToUniformHemispherePdf)
        w[warp.CosineHemisphere] = WarpFactory(SphereWarpAdapter,
            "Square to cosine hemisphere",
            warpWithUnitWeight(warp.squareToCosineHemisphere),
            warp.squareToCosineHemispherePdf)
        w[warp.UniformCone] = WarpFactory(SphereWarpAdapter,
            "Square to uniform cone",
            warpWithUnitWeight(warp.squareToUniformCone),
            warp.squareToUniformConePdf,
            arguments = [WarpAdapter.Argument("cosCutoff", defaultValue = 0.5, description = "Cosine cutoff")])

        # 1D -> 1D warps
        # TODO
        # w[warp.NonUniformTent] = WarpFactory(LineWarpAdapter,
        #     "Square to nonuniform tent",
        #     warpWithUnitWeight(warp.squareToNonUniformTent),
        #     warp.squareToNonuniformTentPdf,
        #     [WarpAdapter.Argument("a"), WarpAdapter.Argument("b"), WarpAdapter.Argument("c")])

        self.warps = w

        # Initialize UI elements
        self.warpTypeChanged = True
        self.warpParametersChanged = True
        self.initializeGUI()

    def runTest(self):
        super(WarpVisualizer, self).runTest(
            WarpVisualizer.minExpFrequency, WarpVisualizer.significanceLevel)
        self.setDrawHistogram(True)
        self.window.setVisible(False)

    def makeAdapter(self, warpType, args):
        """Creates a new WarpAdapter corresponding to the selected warping type
        and parameter values.
        `args` will be passed as kwargs to the warping function."""
        return self.warps[warpType].bind(args)

    def mouseButtonEvent(self, p, button, down, modifiers):
        if down and self.isDrawingHistogram():
            self.setDrawHistogram(False);
            self.window.setVisible(True);
            return True;
        return super(WarpVisualizer, self).mouseButtonEvent(p, button, down, modifiers)

    def keyboardEvent(self, key, scancode, action, modifiers):
        if super(WarpVisualizer, self).keyboardEvent(key, scancode,
                                                     action, modifiers):
            return True
        if key == glfw.KEY_ESCAPE and action == glfw.PRESS:
            self.setVisible(False)
            return True
        return False

    def initializeGUI(self):
        # Main window
        window = Window(self, "Warp tester")
        window.setPosition(Vector2i(15, 15))
        window.setLayout(GroupLayout())

        _ = Label(window, "Input point set", "sans-bold")

        # ---------- First panel
        panel = Widget(window)
        panel.setLayout(BoxLayout(Orientation.Horizontal, Alignment.Middle, 0, 20))

        # Point count slider
        pointCountSlider = Slider(panel)
        pointCountSlider.setFixedWidth(55)
        pointCountSlider.setValue(WarpVisualizer.pointCountDefaultValue)
        pointCountSlider.setCallback(lambda _: self.refresh())
        # Companion text box
        pointCountBox = TextBox(panel)
        pointCountBox.setFixedSize(Vector2i(80, 25))

        # Selection of sampling strategy
        samplingTypeBox = ComboBox(window, ["Independent", "Grid", "Stratified"])
        samplingTypeBox.setCallback(lambda _: self.refresh())

        # Selection of warping method
        def warpTypeCallback(_):
            self.warpTypeChanged = True
            self.refresh()

        _ = Label(window, "Warping method", "sans-bold")
        warpingNames = map(lambda w: self.warps[w].name, self.warps.keys())
        warpTypeBox = ComboBox(window, warpingNames)
        warpTypeBox.setCallback(warpTypeCallback)

        # Option to visualize the warped grid
        warpedGridCheckBox = CheckBox(window, "Visualize warped grid")
        warpedGridCheckBox.setCallback(lambda _: self.refresh())

        # ---------- Second panel
        _ = Label(window, "Method parmeters", "sans-bold")

        warpParametersPanel = Widget(window)
        warpParametersPanel.setLayout(BoxLayout(Orientation.Vertical, Alignment.Middle, 0, 20))

        # ---------- Third panel
        _ = Label(window, "BSDF parameters", "sans-bold")
        panel = Widget(window)
        panel.setLayout(BoxLayout(Orientation.Horizontal, Alignment.Middle, 0, 20))

        # Angle BSDF parameter
        angleSlider = Slider(panel)
        angleSlider.setFixedWidth(55)
        angleSlider.setValue(WarpVisualizer.angleDefaultValue)
        angleSlider.setCallback(lambda _: self.refresh())
        # Companion text box
        angleBox = TextBox(panel)
        angleBox.setFixedSize(Vector2i(80, 25))
        angleBox.setUnits(u"\u00B0")
        # Option to visualize the BRDF values
        brdfValuesCheckBox = CheckBox(window, "Visualize BRDF values")
        brdfValuesCheckBox.setCallback(lambda _: self.refresh())

        # Chi-2 test button
        _ = Label(window, u"\u03C7\u00B2 hypothesis test", "sans-bold")
        testButton = Button(window, "Run", entypo.ICON_CHECK)
        testButton.setBackgroundColor(Color(0, 1., 0., 0.25))

        def tryTest():
            try:
                self.runTest()
            except Exception as e:
                _ = MessageDialog(self, MessageDialog.Type.Warning, "Error",
                                  "An error occurred: " + str(e))
        testButton.setCallback(tryTest)

        # Keep references to the important UI elements
        self.window = window
        self.pointCountSlider = pointCountSlider
        self.pointCountBox = pointCountBox
        self.samplingTypeBox = samplingTypeBox
        self.warpTypeBox = warpTypeBox
        self.warpedGridCheckBox = warpedGridCheckBox
        self.warpParametersPanel = warpParametersPanel
        self.angleSlider = angleSlider
        self.angleBox = angleBox
        self.brdfValuesCheckBox = brdfValuesCheckBox
        self.testButton = testButton

        self.setupSlidersForWarpType(warp.NoWarp)

        self.performLayout()
        self.refresh()
        self.drawAll()
        self.setVisible(True)

    def setupSlidersForWarpType(self, warpType):
        # Clear the previous sliders
        while self.warpParametersPanel.childCount() > 0:
            self.warpParametersPanel.removeChild(0)

        self.parameterSliders = dict()
        arguments = self.warps[warpType].arguments

        for arg in arguments:
            ppanel = Widget(self.warpParametersPanel)
            ppanel.setLayout(BoxLayout(Orientation.Vertical, Alignment.Minimum, 0, 20))
            _ = Label(ppanel, arg.description, "sans-bold")

            panel = Widget(self.warpParametersPanel)
            panel.setLayout(BoxLayout(Orientation.Horizontal, Alignment.Middle, 0, 20))

            slider = Slider(panel)
            slider.setFixedWidth(55)
            slider.setValue(arg.normalize(arg.defaultValue))
            def sliderCallback(_):
                self.warpParametersChanged = True
                self.refresh()
            slider.setCallback(sliderCallback)
            # Companion text box
            box = TextBox(panel)
            box.setFixedSize(Vector2i(80, 25))

            self.parameterSliders[arg.name] = (slider, box)

        self.performLayout()
        self.refresh()

    def refresh(self):
        # TODO: rate limit on refreshes

        samplingType = SamplingType(self.samplingTypeBox.selectedIndex())
        warpType = self.warps.keys()[self.warpTypeBox.selectedIndex()]
        # angle = 180 * self.angleSlider.value() - 90
        # self.angleBox.setValue("{:.1f}".format(angle))

        # Point count slider input is not linear
        pointCount = int(math.pow(2.0, 15.0 * self.pointCountSlider.value() + 5))
        self.pointCountSlider.setValue(
            (math.log(pointCount) / math.log(2.0) - 5) / 15.0);
        # Update the companion box
        def formattedPointCount(n):
            if (n >= 1e6):
                self.pointCountBox.setUnits("M")
                return "{:.2f}".format(n * 1e-6)
            if (n >= 1e3):
                self.pointCountBox.setUnits("K")
                return "{:.2f}".format(n * 1e-3)

            self.pointCountBox.setUnits(" ")
            return str(n)
        self.pointCountBox.setValue(formattedPointCount(pointCount))

        # Now trigger refresh in WarpVisualizationWidget
        self.setSamplingType(samplingType)
        self.setPointCount(pointCount)
        self.setDrawGrid(self.warpedGridCheckBox.checked())

        # TODO: this will be built-in the described arguments
        self.angleSlider.setEnabled(False)
        self.angleBox.setEnabled(False)
        self.brdfValuesCheckBox.setEnabled(False)

        def updateWarpAdapter():
            # Build the arguments
            args = dict()
            for i in range(len(self.warps[warpType].arguments)):
                arg = self.warps[warpType].arguments[i]
                # Actual value needs to be mapped on the range
                value = arg.map(self.parameterSliders[arg.name][0].value())
                args[arg.name] = value

            self.setWarpAdapter(self.makeAdapter(warpType, args))

        if self.warpParametersChanged:
            self.warpParametersChanged = False
            updateWarpAdapter()

        if self.warpTypeChanged:
            self.warpTypeChanged = False
            self.setupSlidersForWarpType(warpType)
            updateWarpAdapter()

        # Update companion boxes for each parameter
        for i in range(len(self.warps[warpType].arguments)):
            arg = self.warps[warpType].arguments[i]
            (slider, box) = self.parameterSliders[arg.name]
            box.setValue("{:.2g}".format(arg.map(slider.value())))

        super(WarpVisualizer, self).refresh()

    def draw(self, ctx):
        super(WarpVisualizer, self).draw(ctx)


if __name__ == "__main__":
    nanogui.init()
    _ = WarpVisualizer()
    nanogui.mainloop()
    gc.collect()
    nanogui.shutdown()
