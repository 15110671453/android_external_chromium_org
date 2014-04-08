

  // upgrade polymer-body last so that it can contain other imported elements
  document.addEventListener('polymer-ready', function() {

    Polymer('polymer-body', Platform.mixin({

      created: function() {
        this.template = document.createElement('template');
        var body = wrap(document).body;
        var c$ = body.childNodes.array();
        for (var i=0, c; (c=c$[i]); i++) {
          if (c.localName !== 'script') {
            this.template.content.appendChild(c);
          }
        }
        // snarf up user defined model
        window.model = this;
      },

      parseDeclaration: function(elementElement) {
        this.lightFromTemplate(this.template);
      }

    }, window.model));

  });

  ;
/**
 * @license
 * Copyright (c) 2012-2014 The Polymer Authors. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *    * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
// @version: 0.2.2
Polymer={},"function"==typeof window.Polymer&&(Polymer={}),function(a){function b(a,b){return a&&b&&Object.getOwnPropertyNames(b).forEach(function(c){var d=Object.getOwnPropertyDescriptor(b,c);d&&(Object.defineProperty(a,c,d),"function"==typeof d.value&&(d.value.nom=c))}),a}a.extend=b}(Polymer),function(a){function b(a,b,d){return a?a.stop():a=new c(this),a.go(b,d),a}var c=function(a){this.context=a,this.boundComplete=this.complete.bind(this)};c.prototype={go:function(a,b){this.callback=a;var c;b?(c=setTimeout(this.boundComplete,b),this.handle=function(){clearTimeout(c)}):(c=requestAnimationFrame(this.boundComplete),this.handle=function(){cancelAnimationFrame(c)})},stop:function(){this.handle&&(this.handle(),this.handle=null)},complete:function(){this.handle&&(this.stop(),this.callback.call(this.context))}},a.job=b}(Polymer),function(){var a={};HTMLElement.register=function(b,c){a[b]=c},HTMLElement.getPrototypeForTag=function(b){var c=b?a[b]:HTMLElement.prototype;return c||Object.getPrototypeOf(document.createElement(b))};var b=Event.prototype.stopPropagation;Event.prototype.stopPropagation=function(){this.cancelBubble=!0,b.apply(this,arguments)}}(Polymer),function(a){function b(a){var c=b.caller,g=c.nom,h=c._super;if(h||(g||(g=c.nom=e.call(this,c)),g||console.warn("called super() on a method not installed declaratively (has no .nom property)"),h=d(c,g,f(this))),h){var i=h[g];return i._super||d(i,g,h),i.apply(this,a||[])}}function c(a,b,c){for(;a;){if(a[b]!==c&&a[b])return a;a=f(a)}}function d(a,b,d){return a._super=c(d,b,a),a._super&&(a._super[b].nom=b),a._super}function e(a){for(var b=this.__proto__;b&&b!==HTMLElement.prototype;){for(var c,d=Object.getOwnPropertyNames(b),e=0,f=d.length;f>e&&(c=d[e]);e++){var g=Object.getOwnPropertyDescriptor(b,c);if("function"==typeof g.value&&g.value===a)return c}b=b.__proto__}}function f(a){return a.__proto__}a.super=b}(Polymer),function(a){function b(a,b){var d=typeof b;return b instanceof Date&&(d="date"),c[d](a,b)}var c={string:function(a){return a},date:function(a){return new Date(Date.parse(a)||Date.now())},"boolean":function(a){return""===a?!0:"false"===a?!1:!!a},number:function(a){var b=parseFloat(a);return 0===b&&(b=parseInt(a)),isNaN(b)?a:b},object:function(a,b){if(null===b)return a;try{return JSON.parse(a.replace(/'/g,'"'))}catch(c){return a}},"function":function(a,b){return b}};a.deserializeValue=b}(Polymer),function(a){var b=a.extend,c={};c.declaration={},c.instance={},c.publish=function(a,c){for(var d in a)b(c,a[d])},a.api=c}(Polymer),function(a){var b={async:function(a,b,c){Platform.flush(),b=b&&b.length?b:[b];var d=function(){(this[a]||a).apply(this,b)}.bind(this),e=c?setTimeout(d,c):requestAnimationFrame(d);return c?e:~e},cancelAsync:function(a){0>a?cancelAnimationFrame(~a):clearTimeout(a)},fire:function(a,b,c,d,e){var f=c||this,b=b||{},g=new CustomEvent(a,{bubbles:void 0!==d?d:!0,cancelable:void 0!==e?e:!0,detail:b});return f.dispatchEvent(g),g},asyncFire:function(){this.async("fire",arguments)},classFollows:function(a,b,c){b&&b.classList.remove(c),a&&a.classList.add(c)}},c=function(){},d={};b.asyncMethod=b.async,a.api.instance.utils=b,a.nop=c,a.nob=d}(Polymer),function(a){var b=window.logFlags||{},c="on-",d={EVENT_PREFIX:c,addHostListeners:function(){var a=this.eventDelegates;b.events&&Object.keys(a).length>0&&console.log("[%s] addHostListeners:",this.localName,a);var d,e,f=this;for(var g in a)e=c+g,(d=PolymerExpressions.prepareEventBinding(Path.get(a[g]),e,{resolveEventHandler:function(a,b){var c=b.getValueFrom(f);return c?c.bind(f):void 0}}))(this,this,!1)},dispatchMethod:function(a,c,d){if(a){b.events&&console.group("[%s] dispatch [%s]",a.localName,c);var e="function"==typeof c?c:a[c];e&&e[d?"apply":"call"](a,d),b.events&&console.groupEnd(),Platform.flush()}}};a.api.instance.events=d}(Polymer),function(a){var b={copyInstanceAttributes:function(){var a=this._instanceAttributes;for(var b in a)this.hasAttribute(b)||this.setAttribute(b,a[b])},takeAttributes:function(){if(this._publishLC)for(var a,b=0,c=this.attributes,d=c.length;(a=c[b])&&d>b;b++)this.attributeToProperty(a.name,a.value)},attributeToProperty:function(b,c){var b=this.propertyForAttribute(b);if(b){if(c&&c.search(a.bindPattern)>=0)return;var d=this[b],c=this.deserializeValue(c,d);c!==d&&(this[b]=c)}},propertyForAttribute:function(a){var b=this._publishLC&&this._publishLC[a];return b},deserializeValue:function(b,c){return a.deserializeValue(b,c)},serializeValue:function(a,b){return"boolean"===b?a?"":void 0:"object"!==b&&"function"!==b&&void 0!==a?a:void 0},reflectPropertyToAttribute:function(a){var b=typeof this[a],c=this.serializeValue(this[a],b);void 0!==c?this.setAttribute(a,c):"boolean"===b&&this.removeAttribute(a)}};a.api.instance.attributes=b}(Polymer),function(a){function b(a,b,d){c.bind&&console.log(e,inB.localName||"object",inPath,a.localName,b);var f=d.discardChanges();return(null===f||void 0===f)&&d.setValue(a[b]),Observer.defineComputedProperty(a,b,d)}var c=window.logFlags||{},d={observeProperties:function(){var a=this._observeNames,b=this._publishNames;if(a&&a.length||b&&b.length){var c=this._propertyObserver=new CompoundObserver;this.registerObservers([c]);for(var d,e=0,f=a.length;f>e&&(d=a[e]);e++){c.addPath(this,d);var g=Object.getOwnPropertyDescriptor(this.__proto__,d);g&&g.value&&this.observeArrayValue(d,g.value,null)}for(var d,e=0,f=b.length;f>e&&(d=b[e]);e++)this.observe&&void 0!==this.observe[d]||c.addPath(this,d);c.open(this.notifyPropertyChanges,this)}},notifyPropertyChanges:function(a,b,c){var d,e,f={};for(var g in b)d=c[2*g+1],void 0!==this.publish[d]&&this.reflectPropertyToAttribute(d),e=this.observe[d],e&&(this.observeArrayValue(d,a[g],b[g]),f[e]||(f[e]=!0,this.invokeMethod(e,[b[g],a[g],arguments])))},observeArrayValue:function(a,b,d){var e=this.observe[a];if(e&&(Array.isArray(d)&&(c.observe&&console.log("[%s] observeArrayValue: unregister observer [%s]",this.localName,a),this.closeNamedObserver(a+"__array")),Array.isArray(b))){c.observe&&console.log("[%s] observeArrayValue: register observer [%s]",this.localName,a,b);var f=new ArrayObserver(b);f.open(function(a,b){this.invokeMethod(e,[b])},this),this.registerNamedObserver(a+"__array",f)}},bindProperty:function(a,c){return b(this,a,c)},invokeMethod:function(a,b){var c=this[a]||a;"function"==typeof c&&c.apply(this,b)},registerObservers:function(a){this._observers.push(a)},closeObservers:function(){for(var a=0,b=this._observers.length;b>a;a++)this.closeObserverArray(this._observers[a]);this._observers=[]},closeObserverArray:function(a){for(var b,c=0,d=a.length;d>c;c++)b=a[c],b&&b.close&&b.close()},registerNamedObserver:function(a,b){var c=this._namedObservers||(this._namedObservers={});c[a]=b},closeNamedObserver:function(a){var b=this._namedObservers;return b&&b[a]?(b[a].close(),b[a]=null,!0):void 0},closeNamedObservers:function(){if(this._namedObservers){for(var a,b,c=Object.keys(this._namedObservers),d=0,e=c.length;e>d&&(a=c[d]);d++)b=this._namedObservers[a],b.close();this._namedObservers={}}}},e="[%s]: bindProperties: [%s] to [%s].[%s]";a.api.instance.properties=d}(Polymer),function(a){function b(a){for(;a.parentNode;){if(a.lightDomController)return a;a=a.parentNode}return a.host}var c=window.logFlags||0,d=(a.api.instance.events,new PolymerExpressions);d.resolveEventHandler=function(a,c,d){var e=b(d);if(e){var f=c.getValueFrom(e);if(f)return f.bind(e)}};var e={syntax:d,instanceTemplate:function(a){var b=a.createInstance(this,this.syntax);return this.registerObservers(b.bindings_),b},bind:function(a,b){var c=this.propertyForAttribute(a);if(c){var d=this.bindProperty(c,b);return this.reflectPropertyToAttribute(c),Platform.enableBindingsReflection&&(d.path=b.path_,this.bindings_=this.bindings_||{},this.bindings_[a]=d),d}return this.mixinSuper(arguments)},asyncUnbindAll:function(){this._unbound||(c.unbind&&console.log("[%s] asyncUnbindAll",this.localName),this._unbindAllJob=this.job(this._unbindAllJob,this.unbindAll,0))},unbindAll:function(){this._unbound||(this.closeObservers(),this.closeNamedObservers(),this._unbound=!0)},cancelUnbindAll:function(){return this._unbound?void(c.unbind&&console.warn("[%s] already unbound, cannot cancel unbindAll",this.localName)):(c.unbind&&console.log("[%s] cancelUnbindAll",this.localName),void(this._unbindAllJob&&(this._unbindAllJob=this._unbindAllJob.stop())))}},f=/\{\{([^{}]*)}}/;a.bindPattern=f,a.api.instance.mdv=e}(Polymer),function(a){function b(a){return a.hasOwnProperty("PolymerBase")}function c(){}var d={PolymerBase:!0,job:function(a,b,c){if("string"!=typeof a)return Polymer.job.call(this,a,b,c);var d="___"+a;this[d]=Polymer.job.call(this,this[d],b,c)},"super":Polymer.super,created:function(){},ready:function(){},createdCallback:function(){this.templateInstance&&console.warn("Attributes on "+this.localName+" were data bound prior to Polymer upgrading the element. This may result in incorrect binding types."),this.created(),this.prepareElement()},prepareElement:function(){this._elementPrepared=!0,this.shadowRoots={},this._observers=[],this.observeProperties(),this.copyInstanceAttributes(),this.takeAttributes(),this.addHostListeners(),this.parseDeclarations(this.__proto__),this.removeAttribute("unresolved"),this.ready()},attachedCallback:function(){this.cancelUnbindAll(),this.attached&&this.attached(),this.enteredView&&this.enteredView(),this.hasBeenAttached||(this.hasBeenAttached=!0,this.domReady&&this.async("domReady"))},detachedCallback:function(){this.preventDispose||this.asyncUnbindAll(),this.detached&&this.detached(),this.leftView&&this.leftView()},enteredViewCallback:function(){this.attachedCallback()},leftViewCallback:function(){this.detachedCallback()},enteredDocumentCallback:function(){this.attachedCallback()},leftDocumentCallback:function(){this.detachedCallback()},parseDeclarations:function(a){a&&a.element&&(this.parseDeclarations(a.__proto__),a.parseDeclaration.call(this,a.element))},parseDeclaration:function(a){var b=this.fetchTemplate(a);if(b){var c=this.shadowFromTemplate(b);this.shadowRoots[a.name]=c}},fetchTemplate:function(a){return a.querySelector("template")},shadowFromTemplate:function(a){if(a){var b=this.createShadowRoot(),c=this.instanceTemplate(a);return b.appendChild(c),this.shadowRootReady(b,a),b}},lightFromTemplate:function(a,b){if(a){this.lightDomController=!0;var c=this.instanceTemplate(a);return b?this.insertBefore(c,b):this.appendChild(c),this.shadowRootReady(this),c}},shadowRootReady:function(a){this.marshalNodeReferences(a),PointerGestures.register(a)},marshalNodeReferences:function(a){var b=this.$=this.$||{};if(a)for(var c,d=a.querySelectorAll("[id]"),e=0,f=d.length;f>e&&(c=d[e]);e++)b[c.id]=c},attributeChangedCallback:function(a){"class"!==a&&"style"!==a&&this.attributeToProperty(a,this.getAttribute(a)),this.attributeChanged&&this.attributeChanged.apply(this,arguments)},onMutation:function(a,b){var c=new MutationObserver(function(a){b.call(this,c,a),c.disconnect()}.bind(this));c.observe(a,{childList:!0,subtree:!0})}};c.prototype=d,d.constructor=c,a.Base=c,a.isBase=b,a.api.instance.base=d}(Polymer),function(a){function b(a){return a.__proto__}function c(a,b){var c="",d=!1;b&&(c=b.localName,d=b.hasAttribute("is"));var e=Platform.ShadowCSS.makeScopeSelector(c,d);return Platform.ShadowCSS.shimCssText(a,e)}var d=(window.logFlags||{},"element"),e="controller",f={STYLE_SCOPE_ATTRIBUTE:d,installControllerStyles:function(){var a=this.findStyleScope();if(a&&!this.scopeHasNamedStyle(a,this.localName)){for(var c=b(this),d="";c&&c.element;)d+=c.element.cssTextForScope(e),c=b(c);d&&this.installScopeCssText(d,a)}},installScopeStyle:function(a,b,c){var c=c||this.findStyleScope(),b=b||"";if(c&&!this.scopeHasNamedStyle(c,this.localName+b)){var d="";if(a instanceof Array)for(var e,f=0,g=a.length;g>f&&(e=a[f]);f++)d+=e.textContent+"\n\n";else d=a.textContent;this.installScopeCssText(d,c,b)}},installScopeCssText:function(a,b,d){if(b=b||this.findStyleScope(),d=d||"",b){window.ShadowDOMPolyfill&&(a=c(a,b.host));var f=this.element.cssTextToScopeStyle(a,e);Polymer.applyStyleToScope(f,b),b._scopeStyles[this.localName+d]=!0}},findStyleScope:function(a){for(var b=a||this;b.parentNode;)b=b.parentNode;return b},scopeHasNamedStyle:function(a,b){return a._scopeStyles=a._scopeStyles||{},a._scopeStyles[b]}};a.api.instance.styles=f}(Polymer),function(a){function b(a,b){if(1===arguments.length&&"string"!=typeof arguments[0]){b=a;var c=document._currentScript;if(a=c&&c.parentNode&&c.parentNode.getAttribute?c.parentNode.getAttribute("name"):"",!a)throw"Element name could not be inferred."}if(f[a])throw"Already registered (Polymer) prototype for element "+a;e(a,b),d(a)}function c(a,b){h[a]=b}function d(a){h[a]&&(h[a].registerWhenReady(),delete h[a])}function e(a,b){return i[a]=b||{}}function f(a){return i[a]}var g=a.extend,h=(a.api,{}),i={};a.getRegisteredPrototype=f,a.waitingForPrototype=c,window.Polymer=b,g(Polymer,a);var j=Platform.deliverDeclarations();if(j)for(var k,l=0,m=j.length;m>l&&(k=j[l]);l++)b.apply(null,k)}(Polymer),function(a){var b={resolveElementPaths:function(a){Platform.urlResolver.resolveDom(a)},addResolvePathApi:function(){var a=this.getAttribute("assetpath")||"",b=new URL(a,this.ownerDocument.baseURI);this.prototype.resolvePath=function(a,c){var d=new URL(a,c||b);return d.href}}};a.api.declaration.path=b}(Polymer),function(a){function b(a,b){var c=new URL(a.getAttribute("href"),b).href;return"@import '"+c+"';"}function c(a,b){if(a){b===document&&(b=document.head),window.ShadowDOMPolyfill&&(b=document.head);var c=d(a.textContent),e=a.getAttribute(h);e&&c.setAttribute(h,e);var f=b.firstElementChild;if(b===document.head){var g="style["+h+"]",i=document.head.querySelectorAll(g);i.length&&(f=i[i.length-1].nextElementSibling)}b.insertBefore(c,f)}}function d(a,b){b=b||document,b=b.createElement?b:b.ownerDocument;var c=b.createElement("style");return c.textContent=a,c}function e(a){return a&&a.__resource||""}function f(a,b){return p?p.call(a,b):void 0}var g=(window.logFlags||{},a.api.instance.styles),h=g.STYLE_SCOPE_ATTRIBUTE,i="style",j="@import",k="link[rel=stylesheet]",l="global",m="polymer-scope",n={loadStyles:function(a){var b=this.templateContent();b&&this.convertSheetsToStyles(b);var c=this.findLoadableStyles(b);c.length?Platform.styleResolver.loadStyles(c,a):a&&a()},convertSheetsToStyles:function(a){for(var c,e,f=a.querySelectorAll(k),g=0,h=f.length;h>g&&(c=f[g]);g++)e=d(b(c,this.ownerDocument.baseURI),this.ownerDocument),this.copySheetAttributes(e,c),c.parentNode.replaceChild(e,c)},copySheetAttributes:function(a,b){for(var c,d=0,e=b.attributes,f=e.length;(c=e[d])&&f>d;d++)"rel"!==c.name&&"href"!==c.name&&a.setAttribute(c.name,c.value)},findLoadableStyles:function(a){var b=[];if(a)for(var c,d=a.querySelectorAll(i),e=0,f=d.length;f>e&&(c=d[e]);e++)c.textContent.match(j)&&b.push(c);return b},installSheets:function(){this.cacheSheets(),this.cacheStyles(),this.installLocalSheets(),this.installGlobalStyles()},cacheSheets:function(){this.sheets=this.findNodes(k),this.sheets.forEach(function(a){a.parentNode&&a.parentNode.removeChild(a)})},cacheStyles:function(){this.styles=this.findNodes(i+"["+m+"]"),this.styles.forEach(function(a){a.parentNode&&a.parentNode.removeChild(a)})},installLocalSheets:function(){var a=this.sheets.filter(function(a){return!a.hasAttribute(m)}),b=this.templateContent();if(b){var c="";if(a.forEach(function(a){c+=e(a)+"\n"}),c){var f=d(c,this.ownerDocument);b.insertBefore(f,b.firstChild)}}},findNodes:function(a,b){var c=this.querySelectorAll(a).array(),d=this.templateContent();if(d){var e=d.querySelectorAll(a).array();c=c.concat(e)}return b?c.filter(b):c},templateContent:function(){var a=this.querySelector("template");return a&&templateContent(a)},installGlobalStyles:function(){var a=this.styleForScope(l);c(a,document.head)},cssTextForScope:function(a){var b="",c="["+m+"="+a+"]",d=function(a){return f(a,c)},g=this.sheets.filter(d);g.forEach(function(a){b+=e(a)+"\n\n"});var h=this.styles.filter(d);return h.forEach(function(a){b+=a.textContent+"\n\n"}),b},styleForScope:function(a){var b=this.cssTextForScope(a);return this.cssTextToScopeStyle(b,a)},cssTextToScopeStyle:function(a,b){if(a){var c=d(a);return c.setAttribute(h,this.getAttribute("name")+"-"+b),c}}},o=HTMLElement.prototype,p=o.matches||o.matchesSelector||o.webkitMatchesSelector||o.mozMatchesSelector;a.api.declaration.styles=n,a.applyStyleToScope=c}(Polymer),function(a){var b=(window.logFlags||{},a.api.instance.events),c=b.EVENT_PREFIX,d={parseHostEvents:function(){var a=this.prototype.eventDelegates;this.addAttributeDelegates(a)},addAttributeDelegates:function(a){for(var b,c=0;b=this.attributes[c];c++)this.hasEventPrefix(b.name)&&(a[this.removeEventPrefix(b.name)]=b.value.replace("{{","").replace("}}","").trim())},hasEventPrefix:function(a){return a&&"o"===a[0]&&"n"===a[1]&&"-"===a[2]},removeEventPrefix:function(a){return a.slice(e)}},e=c.length;a.api.declaration.events=d}(Polymer),function(a){var b={inferObservers:function(a){var b,c=a.observe;for(var d in a)"Changed"===d.slice(-7)&&(c||(c=a.observe={}),b=d.slice(0,-7),c[b]=c[b]||d)},explodeObservers:function(a){var b=a.observe;if(b){var c={};for(var d in b)for(var e,f=d.split(" "),g=0;e=f[g];g++)c[e]=b[d];a.observe=c}},optimizePropertyMaps:function(a){if(a.observe){var b=a._observeNames=[];for(var c in a.observe)for(var d,e=c.split(" "),f=0;d=e[f];f++)b.push(d)}if(a.publish){var b=a._publishNames=[];for(var c in a.publish)b.push(c)}},publishProperties:function(a,b){var c=a.publish;c&&(this.requireProperties(c,a,b),a._publishLC=this.lowerCaseMap(c))},requireProperties:function(a,b,c){for(var d in a)void 0===b[d]&&void 0===c[d]&&(b[d]=a[d])},lowerCaseMap:function(a){var b={};for(var c in a)b[c.toLowerCase()]=c;return b}};a.api.declaration.properties=b}(Polymer),function(a){var b="attributes",c=/\s|,/,d={inheritAttributesObjects:function(a){this.inheritObject(a,"publishLC"),this.inheritObject(a,"_instanceAttributes")},publishAttributes:function(a,d){var e=this.getAttribute(b);if(e)for(var f,g=a.publish||(a.publish={}),h=e.split(c),i=0,j=h.length;j>i;i++)f=h[i].trim(),f&&void 0===g[f]&&void 0===d[f]&&(g[f]=null)},accumulateInstanceAttributes:function(){for(var a,b=this.prototype._instanceAttributes,c=this.attributes,d=0,e=c.length;e>d&&(a=c[d]);d++)this.isInstanceAttribute(a.name)&&(b[a.name]=a.value)},isInstanceAttribute:function(a){return!this.blackList[a]&&"on-"!==a.slice(0,3)},blackList:{name:1,"extends":1,constructor:1,noscript:1,assetpath:1,"cache-csstext":1}};d.blackList[b]=1,a.api.declaration.attributes=d}(Polymer),function(a){function b(a){if(!Object.__proto__){var b=Object.getPrototypeOf(a);a.__proto__=b,d(b)&&(b.__proto__=Object.getPrototypeOf(b))}}var c=a.api,d=a.isBase,e=a.extend,f={register:function(a,b){this.buildPrototype(a,b),this.registerPrototype(a,b),this.publishConstructor()},buildPrototype:function(b,c){var d=a.getRegisteredPrototype(b),e=this.generateBasePrototype(c);this.desugarBeforeChaining(d,e),this.prototype=this.chainPrototypes(d,e),this.desugarAfterChaining(b,c)},desugarBeforeChaining:function(a,b){a.element=this,this.publishAttributes(a,b),this.publishProperties(a,b),this.inferObservers(a),this.explodeObservers(a)},chainPrototypes:function(a,c){this.inheritMetaData(a,c);var d=this.chainObject(a,c);return b(d),d},inheritMetaData:function(a,b){this.inheritObject("observe",a,b),this.inheritObject("publish",a,b),this.inheritObject("_publishLC",a,b),this.inheritObject("_instanceAttributes",a,b),this.inheritObject("eventDelegates",a,b)},desugarAfterChaining:function(a,b){this.optimizePropertyMaps(this.prototype),this.installSheets(),this.resolveElementPaths(this),this.accumulateInstanceAttributes(),this.parseHostEvents(),this.addResolvePathApi(),window.ShadowDOMPolyfill&&Platform.ShadowCSS.shimStyling(this.templateContent(),a,b),this.prototype.registerCallback&&this.prototype.registerCallback(this)},publishConstructor:function(){var a=this.getAttribute("constructor");a&&(window[a]=this.ctor)},generateBasePrototype:function(a){var b=this.findBasePrototype(a);if(!b){var b=HTMLElement.getPrototypeForTag(a);b=this.ensureBaseApi(b),g[a]=b}return b},findBasePrototype:function(a){return g[a]},ensureBaseApi:function(a){if(a.PolymerBase)return a;var b=Object.create(a);return c.publish(c.instance,b),this.mixinMethod(b,a,c.instance.mdv,"bind"),b},mixinMethod:function(a,b,c,d){var e=function(a){return b[d].apply(this,a)};a[d]=function(){return this.mixinSuper=e,c[d].apply(this,arguments)}},inheritObject:function(a,b,c){var d=b[a]||{};b[a]=this.chainObject(d,c[a])},registerPrototype:function(a,b){var c={prototype:this.prototype},d=this.findTypeExtension(b);d&&(c.extends=d),HTMLElement.register(a,this.prototype),this.ctor=document.registerElement(a,c)},findTypeExtension:function(a){if(a&&a.indexOf("-")<0)return a;var b=this.findBasePrototype(a);return b.element?this.findTypeExtension(b.element.extends):void 0}},g={};f.chainObject=Object.__proto__?function(a,b){return a&&b&&a!==b&&(a.__proto__=b),a}:function(a,b){if(a&&b&&a!==b){var c=Object.create(b);a=e(c,a)}return a},c.declaration.prototype=f}(Polymer),function(a){function b(a){return document.contains(a)?g:f}function c(){return f.length?f[0]:g[0]}function d(a){e.waitToReady=!0,CustomElements.ready=!1,HTMLImports.whenImportsReady(function(){e.addReadyCallback(a),e.waitToReady=!1,e.check()})}var e={wait:function(a,b,c){return-1===this.indexOf(a)&&(this.add(a),a.__check=b,a.__go=c),0!==this.indexOf(a)},add:function(a){b(a).push(a)},indexOf:function(a){var c=b(a).indexOf(a);return c>=0&&document.contains(a)&&(c+=HTMLImports.useNative||HTMLImports.ready?f.length:1e9),c},go:function(a){var b=this.remove(a);b&&(b.__go.call(b),b.__check=b.__go=null,this.check())},remove:function(a){var c=this.indexOf(a);if(0===c)return b(a).shift()},check:function(){var a=this.nextElement();return a&&a.__check.call(a),this.canReady()?(this.ready(),!0):void 0},nextElement:function(){return c()},canReady:function(){return!this.waitToReady&&this.isEmpty()},isEmpty:function(){return!f.length&&!g.length},ready:function(){if(CustomElements.ready===!1&&(CustomElements.upgradeDocumentTree(document),CustomElements.ready=!0),h)for(var a;h.length;)(a=h.shift())()},addReadyCallback:function(a){a&&h.push(a)},waitToReady:!0},f=[],g=[],h=[];document.addEventListener("WebComponentsReady",function(){CustomElements.ready=!1}),a.queue=e,a.whenPolymerReady=d}(Polymer),function(a){function b(a,b){a?(document.head.appendChild(a),d(b)):b&&b()}function c(a,c){if(a&&a.length){for(var d,e,f=document.createDocumentFragment(),g=0,h=a.length;h>g&&(d=a[g]);g++)e=document.createElement("link"),e.rel="import",e.href=d,f.appendChild(e);b(f,c)}else c&&c()}var d=a.whenPolymerReady;a.import=c,a.importElements=b}(Polymer),function(a){function b(a){return Boolean(HTMLElement.getPrototypeForTag(a))}function c(a){return a&&a.indexOf("-")>=0}var d=a.extend,e=a.api,f=a.queue,g=a.whenPolymerReady,h=a.getRegisteredPrototype,i=a.waitingForPrototype,j=d(Object.create(HTMLElement.prototype),{createdCallback:function(){this.getAttribute("name")&&this.init()},init:function(){this.name=this.getAttribute("name"),this.extends=this.getAttribute("extends"),this.loadResources(),this.registerWhenReady()},registerWhenReady:function(){this.registered||this.waitingForPrototype(this.name)||this.waitingForQueue()||this.waitingForResources()||f.go(this)},_register:function(){c(this.extends)&&!b(this.extends)&&console.warn("%s is attempting to extend %s, an unregistered element or one that was not registered with Polymer.",this.name,this.extends),this.register(this.name,this.extends),this.registered=!0},waitingForPrototype:function(a){return h(a)?void 0:(i(a,this),this.handleNoScript(a),!0)},handleNoScript:function(a){if(this.hasAttribute("noscript")&&!this.noscript)if(this.noscript=!0,window.CustomElements&&!CustomElements.useNative)Polymer(a);else{var b=document.createElement("script");b.textContent="Polymer('"+a+"');",this.appendChild(b)}},waitingForResources:function(){return this._needsResources},waitingForQueue:function(){return f.wait(this,this.registerWhenReady,this._register)},loadResources:function(){this._needsResources=!0,this.loadStyles(function(){this._needsResources=!1,this.registerWhenReady()}.bind(this))}});e.publish(e.declaration,j),a.getRegisteredPrototype=h,g(function(){document.body.removeAttribute("unresolved"),document.dispatchEvent(new CustomEvent("polymer-ready",{bubbles:!0}))}),document.registerElement("polymer-element",{prototype:j})}(Polymer);
//# sourceMappingURL=polymer.js.map;

    Polymer('polymer-selection', {
      /**
       * If true, multiple selections are allowed.
       *
       * @attribute multi
       * @type boolean
       * @default false
       */
      multi: false,
      ready: function() {
        this.clear();
      },
      clear: function() {
        this.selection = [];
      },
      /**
       * Retrieves the selected item(s).
       * @method getSelection
       * @returns Returns the selected item(s). If the multi property is true,
       * getSelection will return an array, otherwise it will return
       * the selected item or undefined if there is no selection.
      */
      getSelection: function() {
        return this.multi ? this.selection : this.selection[0];
      },
      /**
       * Indicates if a given item is selected.
       * @method isSelected
       * @param {any} item The item whose selection state should be checked.
       * @returns Returns true if `item` is selected.
      */
      isSelected: function(item) {
        return this.selection.indexOf(item) >= 0;
      },
      setItemSelected: function(item, isSelected) {
        if (item !== undefined && item !== null) {
          if (isSelected) {
            this.selection.push(item);
          } else {
            var i = this.selection.indexOf(item);
            if (i >= 0) {
              this.selection.splice(i, 1);
            }
          }
          this.fire("polymer-select", {isSelected: isSelected, item: item});
        }
      },
      /**
       * Set the selection state for a given `item`. If the multi property
       * is true, then the selected state of `item` will be toggled; otherwise
       * the `item` will be selected.
       * @method select
       * @param {any} item: The item to select.
      */
      select: function(item) {
        if (this.multi) {
          this.toggle(item);
        } else if (this.getSelection() !== item) {
          this.setItemSelected(this.getSelection(), false);
          this.setItemSelected(item, true);
        }
      },
      /**
       * Toggles the selection state for `item`.
       * @method toggle
       * @param {any} item: The item to toggle.
      */
      toggle: function(item) {
        this.setItemSelected(item, !this.isSelected(item));
      }
    });
  ;

    Polymer('polymer-selector', {
      /**
       * Gets or sets the selected element.  Default to use the index
       * of the item element.
       *
       * If you want a specific attribute value of the element to be
       * used instead of index, set "valueattr" to that attribute name.
       *
       * Example:
       *
       *     <polymer-selector valueattr="label" selected="foo">
       *       <div label="foo"></div>
       *       <div label="bar"></div>
       *       <div label="zot"></div>
       *     </polymer-selector>
       *
       * In multi-selection this should be an array of values.
       *
       * Example:
       *
       *     <polymer-selector id="selector" valueattr="label" multi>
       *       <div label="foo"></div>
       *       <div label="bar"></div>
       *       <div label="zot"></div>
       *     </polymer-selector>
       *
       *     this.$.selector.selected = ['foo', 'zot'];
       *
       * @attribute selected
       * @type Object
       * @default null
       */
      selected: null,
      /**
       * If true, multiple selections are allowed.
       *
       * @attribute multi
       * @type boolean
       * @default false
       */
      multi: false,
      /**
       * Specifies the attribute to be used for "selected" attribute.
       *
       * @attribute valueattr
       * @type string
       * @default 'name'
       */
      valueattr: 'name',
      /**
       * Specifies the CSS class to be used to add to the selected element.
       *
       * @attribute selectedClass
       * @type string
       * @default 'polymer-selected'
       */
      selectedClass: 'polymer-selected',
      /**
       * Specifies the property to be used to set on the selected element
       * to indicate its active state.
       *
       * @attribute selectedProperty
       * @type string
       * @default ''
       */
      selectedProperty: '',
      /**
       * Specifies the property to be used to set on the selected element
       * to indicate its active state.
       *
       * @attribute selectedProperty
       * @type string
       * @default 'active'
       */
      selectedAttribute: 'active',
      /**
       * Returns the currently selected element. In multi-selection this returns
       * an array of selected elements.
       *
       * @attribute selectedItem
       * @type Object
       * @default null
       */
      selectedItem: null,
      /**
       * In single selection, this returns the model associated with the
       * selected element.
       *
       * @attribute selectedModel
       * @type Object
       * @default null
       */
      selectedModel: null,
      /**
       * In single selection, this returns the selected index.
       *
       * @attribute selectedIndex
       * @type number
       * @default -1
       */
      selectedIndex: -1,
      /**
       * The target element that contains items.  If this is not set
       * polymer-selector is the container.
       *
       * @attribute target
       * @type Object
       * @default null
       */
      target: null,
      /**
       * This can be used to query nodes from the target node to be used for
       * selection items.  Note this only works if the 'target' property is set.
       *
       * Example:
       *
       *     <polymer-selector target="{{$.myForm}}" itemsSelector="input[type=radio]"></polymer-selector>
       *     <form id="myForm">
       *       <label><input type="radio" name="color" value="red"> Red</label> <br>
       *       <label><input type="radio" name="color" value="green"> Green</label> <br>
       *       <label><input type="radio" name="color" value="blue"> Blue</label> <br>
       *       <p>color = {{color}}</p>
       *     </form>
       *
       * @attribute itemSelector
       * @type string
       * @default ''
       */
      itemsSelector: '',
      /**
       * The event that would be fired from the item element to indicate
       * it is being selected.
       *
       * @attribute activateEvent
       * @type string
       * @default 'tap'
       */
      activateEvent: 'tap',
      notap: false,
      ready: function() {
        this.activateListener = this.activateHandler.bind(this);
        this.observer = new MutationObserver(this.updateSelected.bind(this));
        if (!this.target) {
          this.target = this;
        }
      },
      get items() {
        var nodes = this.target !== this ? (this.itemsSelector ?
            this.target.querySelectorAll(this.itemsSelector) :
                this.target.children) : this.$.items.getDistributedNodes();
        return Array.prototype.filter.call(nodes || [], function(n) {
          return n && n.localName !== 'template';
        });
      },
      targetChanged: function(old) {
        if (old) {
          this.removeListener(old);
          this.observer.disconnect();
        }
        if (this.target) {
          this.addListener(this.target);
          this.observer.observe(this.target, {childList: true});
        }
      },
      addListener: function(node) {
        node.addEventListener(this.activateEvent, this.activateListener);
      },
      removeListener: function(node) {
        node.removeEventListener(this.activateEvent, this.activateListener);
      },
      get selection() {
        return this.$.selection.getSelection();
      },
      selectedChanged: function() {
        this.updateSelected();
      },
      updateSelected: function() {
        this.validateSelected();
        if (this.multi) {
          this.clearSelection();
          this.selected && this.selected.forEach(function(s) {
            this.valueToSelection(s);
          }, this);
        } else {
          this.valueToSelection(this.selected);
        }
      },
      validateSelected: function() {
        // convert to an array for multi-selection
        if (this.multi && !Array.isArray(this.selected) &&
            this.selected !== null && this.selected !== undefined) {
          this.selected = [this.selected];
        }
      },
      clearSelection: function() {
        if (this.multi) {
          this.selection.slice().forEach(function(s) {
            this.$.selection.setItemSelected(s, false);
          }, this);
        } else {
          this.$.selection.setItemSelected(this.selection, false);
        }
        this.selectedItem = null;
        this.$.selection.clear();
      },
      valueToSelection: function(value) {
        var item = (value === null || value === undefined) ?
            null : this.items[this.valueToIndex(value)];
        this.$.selection.select(item);
      },
      updateSelectedItem: function() {
        this.selectedItem = this.selection;
      },
      selectedItemChanged: function() {
        if (this.selectedItem) {
          var t = this.selectedItem.templateInstance;
          this.selectedModel = t ? t.model : undefined;
        } else {
          this.selectedModel = null;
        }
        this.selectedIndex = this.selectedItem ?
            parseInt(this.valueToIndex(this.selected)) : -1;
      },
      valueToIndex: function(value) {
        // find an item with value == value and return it's index
        for (var i=0, items=this.items, c; (c=items[i]); i++) {
          if (this.valueForNode(c) == value) {
            return i;
          }
        }
        // if no item found, the value itself is probably the index
        return value;
      },
      valueForNode: function(node) {
        return node[this.valueattr] || node.getAttribute(this.valueattr);
      },
      // events fired from <polymer-selection> object
      selectionSelect: function(e, detail) {
        this.updateSelectedItem();
        if (detail.item) {
          this.applySelection(detail.item, detail.isSelected);
        }
      },
      applySelection: function(item, isSelected) {
        if (this.selectedClass) {
          item.classList.toggle(this.selectedClass, isSelected);
        }
        if (this.selectedProperty) {
          item[this.selectedProperty] = isSelected;
        }
        if (this.selectedAttribute && item.setAttribute) {
          if (isSelected) {
            item.setAttribute(this.selectedAttribute, '');
          } else {
            item.removeAttribute(this.selectedAttribute);
          }
        }
      },
      // event fired from host
      activateHandler: function(e) {
        if (!this.notap) {
          var i = this.findDistributedTarget(e.target, this.items);
          if (i >= 0) {
            var item = this.items[i];
            var s = this.valueForNode(item) || i;
            if (this.multi) {
              if (this.selected) {
                this.addRemoveSelected(s);
              } else {
                this.selected = [s];
              }
            } else {
              this.selected = s;
            }
            this.asyncFire('polymer-activate', {item: item});
          }
        }
      },
      addRemoveSelected: function(value) {
        var i = this.selected.indexOf(value);
        if (i >= 0) {
          this.selected.splice(i, 1);
        } else {
          this.selected.push(value);
        }
        this.valueToSelection(value);
      },
      findDistributedTarget: function(target, nodes) {
        // find first ancestor of target (including itself) that
        // is in nodes, if any
        while (target && target != this) {
          var i = Array.prototype.indexOf.call(nodes, target);
          if (i >= 0) {
            return i;
          }
          target = target.parentNode;
        }
      }
    });
