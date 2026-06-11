import QtQml 2.0
import qf.qmlreports 1.0
import shared.qml.reports 1.0
import "qrc:/quickevent/core/js/ogtime.js" as OGTime

Report {
	id: root
	objectName: "root"

	property string reportTitle: qsTr("Awards")
	property var eventConfig

	//debugLevel: 1
	styleSheet: StyleSheet {
		objectName: "portraitStyleSheet"
		basedOn: ReportStyleCommon { id: myStyle }
		colors: [
			Color { id: colorMaroon; name: "maroon"; def: "maroon" }
		]
		fonts: [
			Font {
				id: fontTitle
				family: "Times"
				hint: Font.HintSerif
				pointSize: myStyle.textStyleDefault.font.pointSize * 10
			},
			Font {
				id: fontEventName
				pointSize: myStyle.textStyleDefault.font.pointSize * 2.5
			},
			Font {
				id: fontPosition
				pointSize: myStyle.textStyleDefault.font.pointSize * 2.5
			},
			Font {
				id: fontClub
				pointSize: myStyle.textStyleDefault.font.pointSize * 2.5
			},
			Font {
				id: fontRunners
				pointSize: myStyle.textStyleDefault.font.pointSize * 2
			}
		]
		textStyles: [
			TextStyle {
				id: tsTitle
				name: "title"
				font: fontTitle
				pen: Pen { basedOn: "black1"; color: colorMaroon }
			},
			TextStyle {
				id: tsEventName
				name: "eventName"
				font: fontEventName
			},
			TextStyle {
				id: tsPosition
				name: "position"
				font: fontPosition
			},
			TextStyle {
				id: tsClub
				name: "club"
				font: fontClub
			},
			TextStyle {
				id: tsRunners
				name: "runners"
				font: fontRunners
			}
		]
	}
	textStyle: myStyle.textStyleDefault

	width: 210
	height: 297
	hinset: 10
	vinset: 10
	Frame {
		width: "%"
		height: "%"
		Band {
			id: root_band
			objectName: "band"
			Detail {
				id: class_detail
				Band {
					objectName: "relayBand"
					Detail {
						id: relay_detail
						width: "%"

						Break { skipFirst: true }

						Frame { height: 10 }
						Para {
							width: "%"
							halign: Frame.AlignHCenter
							textStyle: TextStyle { basedOn: tsEventName; font: Font { basedOn: fontEventName; weight: Font.WeightBold } }
							textFn: function() { return root_band.data("event").name; }
						}
						Para {
							width: "%"
							halign: Frame.AlignHCenter
							textStyle: myStyle.textStyleDefault
							textFn: function() {
								var start = root_band.data("stageStart");
								if(!start) {
									var ev = root_band.data("event");
									start = ev.dateTime ? ev.dateTime : ev.date;
								}
								if(start) {
									var d = new Date(start);
									if(!isNaN(d.getTime())) {
										var day = d.getDate();
										var month = d.getMonth() + 1;
										return (day < 10 ? "0" + day : "" + day) + "."
											+ (month < 10 ? "0" + month : "" + month) + "."
											+ d.getFullYear();
									}
								}
								return start ? start : "";
							}
						}
						Frame { height: 20 }
						Para {
							width: "%"
							halign: Frame.AlignHCenter
							textStyle: tsTitle
							text: "Diplom"
						}
						Frame { height: 20 }
						Para {
							width: "%"
							halign: Frame.AlignHCenter
							textStyle: TextStyle { basedOn: tsPosition; font: Font { basedOn: fontPosition; weight: Font.WeightBold } }
							textFn: function() {
								var pos = relay_detail.rowData("pos");
								var cls = class_detail.rowData("className");
								return (pos > 0 ? pos + ". místo" : "místo") + " v kategorii " + cls;
							}
						}
						Frame { height: 10 }
						Para {
							width: "%"
							halign: Frame.AlignHCenter
							textStyle: TextStyle { basedOn: tsClub; font: Font { basedOn: fontClub; weight: Font.WeightBold } }
							textFn: function() { return relay_detail.rowData("orgName"); }
						}
						Frame { height: 8 }
						Band {
							objectName: "runnersBand"
							Detail {
								id: runner_detail
								width: "%"
								Para {
									width: "%"
									halign: Frame.AlignHCenter
									textStyle: tsRunners
									textFn: function() { return runner_detail.rowData("competitorName"); }
								}
							}
						}
						Frame { height: "%" }
						Frame { height: 20 }
						Frame {
							width: "%"
							layout: Frame.LayoutHorizontal
							Frame { width: 10 }
							Frame {
								width: "%"
								Para {
									width: "%"
									halign: Frame.AlignHCenter
									textStyle: myStyle.textStyleBold
									text: root.eventConfig ? root.eventConfig.mainReferee : ""
								}
								Para {
									width: "%"
									topBorder: Pen { basedOn: "black1dot" }
									halign: Frame.AlignHCenter
									text: "Hlavní rozhodčí"
								}
							}
							Frame { width: "40%" }
							Frame {
								width: "%"
								Para {
									width: "%"
									halign: Frame.AlignHCenter
									textStyle: myStyle.textStyleBold
									text: root.eventConfig ? root.eventConfig.director : ""
								}
								Para {
									width: "%"
									topBorder: Pen { basedOn: "black1dot" }
									halign: Frame.AlignHCenter
									text: "Ředitel závodu"
								}
							}
							Frame { width: 10 }
						}
						Frame { height: 10 }
					}
				}
			}
		}
	}
}
