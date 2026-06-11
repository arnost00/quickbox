import QtQml 2.0
import qf.qmlreports 1.0
import shared.qml.reports 1.0

Report {
	id: root
	objectName: "root"

	property string reportTitle: qsTr("Awards")
	property var eventConfig
	property var awardRenderer   // AwardQmlRenderer* passed via report_init_properties

	styleSheet: StyleSheet {
		basedOn: ReportStyleCommon { id: myStyle }
	}
	textStyle: myStyle.textStyleDefault

	width: 210
	height: 297

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

						Image {
							width: "%"
							height: "%"
							aspectRatio: Image.AspectRatioIgnore
							dataFormat: Image.FormatPng
							dataEncoding: Image.EncodingBase64
							dataFn: function() {
								if (!root.awardRenderer)
									return "";
								return root.awardRenderer.renderRunPageBase64(
									class_detail.currentIndex,
									relay_detail.currentIndex
								);
							}
						}
					}
				}
			}
		}
	}
}
