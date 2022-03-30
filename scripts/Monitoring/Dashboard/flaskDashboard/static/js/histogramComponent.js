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