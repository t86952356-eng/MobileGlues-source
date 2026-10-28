package com.fcl.plugin.mobileglues.settings

import com.google.gson.JsonParser
import org.junit.Assert.*
import org.junit.Test

class G99ProfileTest {
    @Test fun directSubmissionDefaultsAndIndependentToggle() {
        assertTrue(MGConfigCodec.decode(JsonParser.parseString("{}").asJsonObject).g99DirectSubmit)
        for (wire in listOf("0", "2", "-1", "true", "\"1\"", "null", "[]")) {
            val decoded = MGConfigCodec.decode(JsonParser.parseString("{\"g99DirectSubmit\":$wire}").asJsonObject)
            assertFalse(decoded.g99DirectSubmit)
        }
        val before = MGConfig.Default.copy(angle = AngleConfig.ForceEnable, glslCache = GlslCacheSize.Limited(256))
        val after = before.copy(g99DirectSubmit = false)
        assertTrue(after.helioG99Test)
        assertEquals(before, after.copy(g99DirectSubmit = true))
        val encoded = MGConfigCodec.encode(after, JsonParser.parseString("{\"futureKey\":42}").asJsonObject)
        assertEquals(0, encoded.get("g99DirectSubmit").asInt)
        assertEquals(after, MGConfigCodec.decode(encoded))
        assertEquals(42, encoded.get("futureKey").asInt)
        assertNull(MGConfigCodec.foreignKeysOf(encoded).get("g99DirectSubmit"))
    }

    @Test fun missingRequestDefaultsOnButExplicitInvalidOrOffDoNot() {
        assertTrue(MGConfigCodec.decode(JsonParser.parseString("{}").asJsonObject).helioG99Test)
        for (value in listOf("{\"helioG99Test\":0}", "{\"helioG99Test\":2}", "{\"helioG99Test\":-1}", "{\"helioG99Test\":true}", "{\"helioG99Test\":\"bad\"}", "{\"helioG99Test\":\"1\"}", "{\"helioG99Test\":null}", "{\"helioG99Test\":[]}")) {
            assertFalse(MGConfigCodec.decode(JsonParser.parseString(value).asJsonObject).helioG99Test)
        }
    }
    @Test fun g99DefaultsAreExplicitAndSafe() {
        val config = MGConfig.Default
        assertTrue(config.helioG99Test)
        assertEquals(AngleConfig.DisableIfPossible, config.angle)
        assertEquals(NoErrorConfig.DoNotIgnore, config.noError)
        assertEquals(64, config.glslCache.wire)
        assertFalse(config.extTimerQuery)
        assertFalse(config.extComputeShader)
        assertFalse(config.extDirectStateAccess)
        assertEquals(Fsr1Preset.Disabled, config.fsr1)
        assertEquals(GlVersion.Default, config.glVersion)
        assertEquals(MultidrawSettings.Default, config.multidraw)
    }
    @Test fun requestUsesNumericNativeWireFormatAndRoundTrips() {
        val config = MGConfig.Default.copy(helioG99Test = true)
        val encoded = MGConfigCodec.encode(config, null)
        assertEquals(1, encoded.get("helioG99Test").asInt)
        assertTrue(MGConfigCodec.decode(encoded).helioG99Test)
        assertEquals(config, MGConfigCodec.decode(encoded))
    }
    @Test fun disablingPreservesEveryManualSetting() {
        val original = MGConfig.Default.copy(helioG99Test = false, angle = AngleConfig.ForceEnable,
            fsr1 = Fsr1Preset.Balanced, glslCache = GlslCacheSize.Limited(256))
        val toggled = original.copy(helioG99Test = true).copy(helioG99Test = false)
        assertEquals(original, toggled)
        assertEquals(original, MGConfigCodec.decode(MGConfigCodec.encode(toggled, null)))
    }
    @Test fun unknownSettingsSurviveAndProfileKeyIsOwned() {
        val raw = JsonParser.parseString("{\"helioG99Test\":1,\"futureKey\":42}").asJsonObject
        val foreign = MGConfigCodec.foreignKeysOf(raw)
        assertNull(foreign.get("helioG99Test"))
        val encoded = MGConfigCodec.encode(MGConfigCodec.decode(raw), foreign)
        assertEquals(42, encoded.get("futureKey").asInt)
        assertEquals(1, encoded.get("helioG99Test").asInt)
    }
}
