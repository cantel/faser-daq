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
