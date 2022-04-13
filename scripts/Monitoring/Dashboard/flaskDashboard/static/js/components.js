// Code for the histogram component 

Vue.component("histogram-card", {
    delimiters: ["[[", "]]"],
    props: ["id", "data"],
    data: function () {
        return {
            c_updating_rate: 5000, // updating every 5 seconds TODO: to be removed if new version work.
            c_timestamp: null,
            c_figure: null,
            c_isLoading: false,
            c_tags: [],
            c_runNumber: null,
            c_flags: [],
        }
    },
    mounted() {
        this.pollData()
    },
    beforeDestroy() {
        clearInterval(this.c_polling)
    },
    filters: {
        timestampLocal: function (timestamp) {
            return dayjs.unix(timestamp).format("HH:mm:ss (DD/MM)")
        },
        timestampCERN: function (timestamp) {
            return dayjs.tz(parseFloat(timestamp) * 1000, "Europe/Zurich").format(
                "HH:mm:ss (DD/MM)")
        }
    },
    computed: {
        divName() {
            return "graph_" + this.id
        },
        flags_style_object() {
            // write simpler code (if only two flags)
            let border_color = "white"
            let outline_color = "white"

            if (this.c_flags) {
                // outline_color = this.c_flags[0]
                let uoverflow = this.c_flags.filter(obj => { return obj.name === "uoverflow" })[0]
                let count_error = this.c_flags.filter(obj => { return obj.name === "count_error" })[0]

                if (uoverflow) {
                    outline_color = uoverflow.color;
                }
                if (count_error) {
                    border_color = count_error.color
                }
                border_color  = uoverflow ? uoverflow.color : "white"
                outline_color = count_error ? count_error.color : "white"

            }
            return {
                border: `6px solid ${border_color}`,
                outline: `4px solid ${outline_color}`,

            }

        },

    },
    methods: {

        pollData() {
            axios.get('/histogram_from_ID', {
                params: {
                    ID: this.id
                }
            })
                .then(response => {
                    this.c_figure = response.data["fig"]
                    this.c_timestamp = response.data["timestamp"]
                    this.c_tags = response.data["tags"]
                    this.c_runNumber = response.data["runNumber"]
                    this.c_flags = response.data["flags"]
                })

        },
        removeTag(item) {

            const index = this.c_tags.indexOf(item);
            if (index > -1) {
                this.c_tags.splice(index, 1);
            }
            axios.post('/remove_tag', {
                ID: this.id,
                tag: item
            })
                .then(response => {
                    if (!response.data["exists"]) {
                        EventBus.$emit("remove_tag_from_dropdown", item)
                    }
                })

        },
        openTimeView() {
            window.open(`/history_view/${this.id}`, "_blank", "width=800, height=500")
        }
    },
    watch: {
        c_tags: {
            handler: function (newVal, oldVal) {
                if (newVal.length > oldVal.length && newVal.length - oldVal.length === 1) {
                    var addedTag = newVal.filter(x => oldVal.indexOf(x) === -1);
                    axios.post('/add_tag', {
                        ID: this.id,
                        tag: addedTag[0]
                    })
                        .then(response => {
                            if (!response.data["existed"]) {
                                EventBus.$emit("add_tag_to_dropdown", addedTag[0])
                            }

                        })
                } else if (newVal.length < oldVal.length && newVal.length - oldVal.length === -1) {
                    var removedTag = oldVal.filter(x => newVal.indexOf(x) === -1);
                    axios.post('/remove_tag', {
                        ID: this.id,
                        tag: removedTag[0]
                    })
                        .then(response => {
                            if (!response.data["exists"]) {
                                EventBus.$emit("remove_tag_from_dropdown", removedTag[0])
                            }
                        })
                }
            }
        },
        data: {
            handler: function (newVal, oldVal) {

                if (oldVal) {
                    // if (newVal.timestamp === oldVal.timestamp) {
                    //     console.log(`${this.id} n'a pas changé`)
                    // }
                    if (newVal.timestamp !== oldVal.timestamp) {
                        // console.log(`${this.id} a changé`)
                        this.c_figure = newVal["fig"];
                        this.c_timestamp = newVal["timestamp"]
                        this.c_tags = newVal["tags"]
                        this.c_runNumber = newVal["runNumber"]
                        // this.c_flags = newVal["flags"]
                    }
                }
            },
            deep: true
        }
    },
    template: //html
        `
    <span>
        <v-card width="500" tile class="mx-auto mt-5" :style="flags_style_object">
            <v-card-title class="pb-1 pt-2">
                <span class="hist_title"> [[ id ]]</span>
                <v-spacer> </v-spacer>
                <v-btn icon @click="openTimeView">
                    <v-icon>mdi-history</v-icon>
                </v-btn>
                <v-spacer> </v-spacer>

                <v-tooltip top transition="fade-transition">
                    <template v-slot:activator="{ on, attrs }">
                        <span class="timestamp" v-bind="attrs" v-on="on">
                            <i>[[ c_timestamp | timestampLocal ]] R[[ c_runNumber ]]</i>
                        </span>
                    </template>
                    <span>CERN: [[ c_timestamp | timestampCERN ]]</span>
                    </v-tooltip>
            </v-card-title>
                <plotly-graph class="card_histo_cont mx-auto" v-if="c_figure" :figure="c_figure"
                    :divid="divName">
                </plotly-graph>
                
                <v-card-actions class="pb-1">
                    <v-combobox v-model="c_tags" small-chips multiple solo hide-selected hide-details
                        persistent-hint dense hide-no-data flat label="Enter tags...">
                    <template v-slot:selection="{ attrs, item, select, selected }">
                        <v-chip close pill x-small v:bind="attrs" :input-value="selected" @click="select" @click:close="removeTag(item)">
                            [[ item ]]
                        </v-chip>                            
                    </template>
                </v-combobox>
            </v-card-actions>
        </v-card>
    </span>
    `,
})

// Plotly-graph component 
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


// redis_info_dialog component 

Vue.component("redis_info_dialog", {
    delimiters: ["[[", "]]"],
    data: function () {
        return {
            dialog: false,
            redis_infos: {}
        }
    },
    mounted() {
        axios.get("/redis_info").then(response => {
            this.redis_infos = response.data
        })
    },
    template: //html
        `
<v-dialog v-model="dialog" width="500">
<template v-slot:activator="{ on, attrs }">
<v-btn plain v-bind="attrs" v-on="on" style="text-transform: none;">
    Redis infos
</v-btn>
</template>
<v-card>
<v-card-title class="headline grey lighten-2">
    Redis stats (summary)
</v-card-title>

<v-card-text>
    <v-list dense>
        <v-list-item two-line>
            <v-list-item-content>
                <v-list-item-title> Memory used </v-list-item-title>
                <v-list-item-subtitle>[[ this.redis_infos.used_memory_human]]</v-list-item-subtitle>
            </v-list-item-content>
        </v-list-item>

        <v-list-item two-line>
            <v-list-item-content>
                <v-list-item-title>Memory used (rss) </v-list-item-title>
                <v-list-item-subtitle>[[ this.redis_infos.used_memory_rss_human ]]</v-list-item-subtitle>
            </v-list-item-content>
        </v-list-item>
    </v-list>
</v-card-text>

<v-divider></v-divider>

<v-card-actions>
    <v-spacer></v-spacer>
    <v-btn color="primary" text @click="dialog = false">
        Close
    </v-btn>
</v-card-actions>
</v-card>
</v-dialog>
`
})


// search-tool component 

Vue.component("search-tool", {
    delimiters: ["[[", "]]"],
    data: function () {
      return {
        selected_tags: [],
        c_tags: [],
        c_modules_tags: [],
        c_selected_module: null,
        c_all_tags: [],
      };
    },
    mounted() {
      axios
        .get("/getModulesAndTags")
        .then((response) => {
          this.c_tags = response.data["modules"]
            .concat(response.data["tags"])
            .sort();
          this.c_modules_tags = response.data["modules"].sort();
        })
        .catch((err) => {
          console.log("Could not get the IDs");
        });
      EventBus.$on("add_tag_to_dropdown", (tag) => {
        this.c_tags.push(tag);
      });
  
      EventBus.$on("remove_tag_from_dropdown", (tag) => {
        const index = this.c_tags.indexOf(tag);
        if (index > -1) {
          this.c_tags.splice(index, 1);
        }
      });
    },
    methods: {
      emitTags: function () {
        this.$emit("search-tags", this.selected_tags);
      },
      emitModule: function () {
        const selected_module = [this.c_selected_module];
        this.$emit("search-tags", selected_module);
      },
    },
    template: /*html*/ `
    <v-container class="d-inline-flex justify-center">
          <v-select
            class="mx-2"
            dense
            flat
            hide-details
            solo
            :append-outer-icon="'mdi-toy-brick-search-outline'"
            @click:append-outer="emitModule"
            :items="c_modules_tags"
            v-model="c_selected_module"
          >
          </v-select>
    
        <v-combobox
          v-model="selected_tags"
          :items="c_tags"
          :append-outer-icon="'mdi-magnify'"
          @click:append-outer="emitTags"
          multiple
          solo
          hide-details
          hide-selected
          clearable
          deletable-chips
          dense
          small-chips
          label="Select or write expression ..."
          flat
          :menu-props="{closeOnClick: true}"
        >
      </v-combobox>
  </v-container>
    `,
  });
  