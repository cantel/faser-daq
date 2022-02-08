Vue.component("search-tool", {
    delimiters: ["[[", "]]"],
    data: function () {
        return {
            selected_tags: [],
            c_tags: [],
            c_modules_tags: [],
            c_selected_module: null,
            c_all_tags: []
        }
    },
    mounted() {
        axios.get("/getModulesAndTags").then(response => {
            this.c_tags = response.data["modules"].concat(response.data["tags"]).sort()
            this.c_modules_tags = response.data["modules"].sort()
        })
            .catch(err => {
                console.log("Could not get the IDs");
            })
        EventBus.$on("add_tag_to_dropdown", (tag) => {
            this.c_tags.push(tag)
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
            this.$emit('search-tags', this.selected_tags)
        },
        emitModule: function () {
            const selected_module = [this.c_selected_module]
            this.$emit('search-tags', selected_module)
        }
    },
    template: /*html*/
        `
    <div class="menu">
        <div style="width:50%;">
            <v-select class="mx-2" dense flat hide-details solo
                :append-outer-icon="'mdi-toy-brick-search-outline'" @click:append-outer="emitModule"
                :items="c_modules_tags" v-model="c_selected_module">
            </v-select>
        </div>
    <v-autocomplete v-model="selected_tags"
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
    label = "select tags ..."
    flat
    :menu-props="{closeOnClick: true}"
    ></v-autocomplete>
    </div>
    `,
})