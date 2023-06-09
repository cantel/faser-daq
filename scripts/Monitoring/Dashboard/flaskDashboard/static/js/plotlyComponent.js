Vue.component("plotly-graph", {
    delimiters: ["[[", "]]"],
    props: ["figure", "divid"],
    data: function () {
        return {
            c_log: false,
            c_first_time: true,
            c_config: {
                toImageButtonOptions: {
                    filename: "default"
                },
                displayModeBar: true,
                displaylogo: false,
                modeBarButtonsToRemove: ['hoverClosestGl2d', 'lasso2d', 'resetScale2d'],
                modeBarButtonsToAdd: [
                    {
                        name: "log scale",
                        icon: { 'width': 24, 'height': 24, 'path': "M18 7C16.9 7 16 7.9 16 9V15C16 16.1 16.9 17 18 17H20C21.1 17 22 16.1 22 15V11H20V15H18V9H22V7H18M2 7V17H8V15H4V7H2M11 7C9.9 7 9 7.9 9 9V15C9 16.1 9.9 17 11 17H13C14.1 17 15 16.1 15 15V9C15 7.9 14.1 7 13 7H11M11 9H13V15H11V9Z" },
                        click: (gd) => {
                            this.c_log = !this.c_log
                            if (this.c_log) {
                                Plotly.relayout(gd, { 'yaxis.type': "log" })
                            }
                            else {
                                Plotly.relayout(gd, { 'yaxis.type': "linear" })
                            }
                        }
                    }
                ]
            },
        }
    },
    mounted() {
        this.$watch('figure', (figure) => {
            if (this.c_log) {
                figure.layout.yaxis.type = "log"
            }
            if (figure.config.filename) {
                var ts = figure.config.filename.split("+")
                var day = dayjs.tz(parseFloat(ts[1]) * 1000, "Europe/Zurich").format("DDMMYYYY_HHmmss")
                this.c_config.toImageButtonOptions.filename = `${ts[0]}_${day}`
            }

            Plotly.react(this.$refs[this.divid], figure.data, figure.layout, this.c_config).then(() => {
                if (this.c_first_time) {
                    EventBus.$emit("finishLoading");
                }
                else {
                    this.c_first_time = false;
                }
            })
        }, {
            immediate: true,
            deep: true
        });
    },
    template: //html
        `
    <div :ref ="divid">
    </div>
    `,
})