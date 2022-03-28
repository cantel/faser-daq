Vue.component("config-panel", {
  delimiters: ["[[", "]]"],
  props: [],
  data: function () {
    return {
      c_loadedConfigName: null,
      c_configs: [],
      dialog: false,
    };
  },
  mounted() {
    axios.get("/configs").then((response) => {
      this.c_configs = response.data;
    });
  },
  methods: {
    loadConfig(item) {
      this.c_loadedConfigName = item;
    },
  },
  template: /*html*/ `
    <v-card tile elevation="1" class="mx-4 my-4">
      <v-card-title> CONFIGURATION </v-card-title>
      <v-card-text>
        <v-row class="text-center">
          <v-menu offset-y>
            <template v-slot:activator="{ on, attrs }">
              <v-btn
                v-bind="attrs"
                v-on="on"
                class="mx-4"
                depressed
                color="primary"
                small
              >
                load
              </v-btn>
            </template>
            <v-list dense>
              <v-list-item-group>
                <v-list-item
                  v-for="(item, index) in c_configs"
                  @click="loadConfig(item)"
                  :key="index"
                >
                  <v-list-item-title> [[item]] </v-list-item-title>
                </v-list-item>
              </v-list-item-group>
            </v-list>
          </v-menu>
          <v-btn class="" depressed color="primary" small> upload </v-btn>

          <div class="mx-10">Loaded config : [[c_loadedConfigName]]</div>
          <json-viewer></json-viewer>

        </v-row>
      </v-card-text>
    </v-card>
  `,
});
