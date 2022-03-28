Vue.component("json-viewer", {
    delimiters: ["[[", "]]"],
    props: [],
    data: function () {
      return {
          dialog: false

      };
    },
    mounted() {},
    methods: {
     
    },
    template: /*html*/`
     <v-dialog transition="fade-transition" max-width="600">
            <template v-slot:activator="{ on, attrs }">
              <v-btn icon text v-bind="attrs" v-on="on">
                <v-icon> mdi-eye </v-icon>
              </v-btn>
            </template>
            <template v-slot:default="dialog">
              <v-card>
                <v-card-text>
                    djfasdf
                    asdfasf
                    sdsdsd
                    d
                    sdsdfasdfsdf asdf a sd fsdf a sdffa sdfsdfasdf sdf sd
                    asdsfasdsfasdfsdf asdf asdf asdf asfd asd ff
                    asdsf a sdffaasdfsdf

                    asdf asdfas
                    asdf asdf    
                </v-card-text>
              </v-card>
            </template>
          </v-dialog>
    `,
  });
  