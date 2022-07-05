Vue.component("info_panel", {
    delimiters: ["[[", "]]"],
    props: {"module": String},
    data: function () {
      return {
        data: [],
        interval : 1000, // 1 second
        c_polling : null
      };
    },
    mounted() {
      this.getData();
    },
    beforeDestroy (){
      clearInterval(this.c_polling);
    },
    methods: {
      getData : function(){
        axios.get(`/info?module=${this.module}`).then((response)=>{
          this.data = response.data;
          
        })
        this.c_polling = setInterval(()=>{
          axios.get(`/info?module=${this.module}`).then((response)=>{
            this.data = response.data;
            
          })
        }, this.interval)
      }
    },
    template: `
    <div>
      <v-simple-table dense v-if="data.length != 0 ">
        <template v-slot:default>
          <thead>
            <tr>
              <th class="text-left">Key</th>
              <th class="text-left">Value</th>
              <th class="text-left">Time</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="item in data" :key="item.key">
              <td>[[ item.key ]]</td>
              <td>[[ item.value ]]</td>
              <td>[[ item.time ]]</td>
            </tr>
          </tbody>
        </template>
      </v-simple-table>
      <div v-else class="text-center flex align-center" style="height: 100%"> <h2>No data to display </h2> </div>
    </div>
    `,
  });
  