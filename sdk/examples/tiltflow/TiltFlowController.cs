using JsonFx.Json;
using Sifteo;
using Sifteo.MathExt;
using Sifteo.Util;
using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace MenuTest {

	public class TiltFlowController : Controller {
		public TiltFlowController(Bootstrap app) : base(app) {
		}

    void AddResult(TiltFlowMenu menu) {
      if (menu.ResultItem.name != "Back") {
        AddResult(menu.ResultItem.image+"_small", menu.ResultItem.name);
      }
    }

    override protected IEnumerator<float> Coroutine() {
    ChooseGender:
      ShowMessage("Choose a Gender");
      yield return 2;
			using (var menu = new TiltFlowMenu(
        app,
        new TiltFlowItem("icon_gender_male", "Boy", "This is the Boy!"),
        new TiltFlowItem("icon_gender_female", "Girl", "This is the Girl!")
      )) {
        while(!menu.Done) {
          yield return 0;
          menu.Tick(app.dt);
        }
        AddResult(menu);
      }
    ChooseElem:
      ShowMessage("Choose an Element");
      yield return 2f;
      using (var menu = new TiltFlowMenu(
        app,
        new TiltFlowItem("icon_elem_water", "Water", "Nice, cool water!"),
        new TiltFlowItem("icon_elem_fire", "Fire", "Burning fire!"),
        new TiltFlowItem("icon_elem_earth", "Earth", "Good old dirt!"),
        new TiltFlowItem("icon_elem_wind", "Wind", "A gentle breeze!"),
        new TiltFlowItem("icon_back", "Back", "Go to the previous menu")
      )) {
        while(!menu.Done) {
          yield return 0;
          menu.Tick(app.dt);
        }
        AddResult(menu);
        if (menu.ResultItem.name == "Back") {
          PopResult();
          goto ChooseGender;
        }
      }
      ShowMessage("Choose a Shape");
      yield return 2f;
      using (var menu = new TiltFlowMenu(
        app,
        new TiltFlowItem("icon_shape_circle", "Circle", "Coming full circle!"),
        new TiltFlowItem("icon_shape_triangle", "Triangle", "Three sides are enough!"),
        new TiltFlowItem("icon_shape_square", "Square", "Dependable four sides!"),
        new TiltFlowItem("icon_shape_cross", "Cross", "Cross my heart!"),
        new TiltFlowItem("icon_shape_diamond", "Diamond", "A priceless diamond!"),
        new TiltFlowItem("icon_shape_star", "Star", "A shooting star!"),
        new TiltFlowItem("icon_shape_hexagon", "Hexagon", "Six sides of power!"),
        new TiltFlowItem("icon_back", "Back", "Go to the previous menu")
      )) {
        while(!menu.Done) {
          yield return 0;
          menu.Tick(app.dt);
        }
        AddResult(menu);
        if (menu.ResultItem.name == "Back") {
          PopResult();
          goto ChooseElem;
        }
      }
		}

	}
}

