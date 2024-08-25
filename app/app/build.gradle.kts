import java.util.Properties

plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

val keystoreProperties = Properties().apply {
    rootProject.file("keystore.properties").inputStream().use { fis ->
        load(fis)
    }
}

android {
    namespace = "org.lindroid.ui"
    compileSdk = 34

    defaultConfig {
        applicationId = "org.lindroid.ui"
        minSdk = 33
        //noinspection OldTargetApi
        targetSdk = 34
        versionCode = 34
        versionName = "14"
    }

    buildFeatures {
        aidl = true
    }
    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro",
            )
            signingConfig = signingConfigs["debug"]
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
    kotlinOptions {
        jvmTarget = "17"
        freeCompilerArgs += listOf(
            "-Xno-param-assertions",
            "-Xno-call-assertions",
            "-Xno-receiver-assertions",
        )
    }
    signingConfigs {
        named("debug") {
            keyAlias = keystoreProperties["keyAlias"].toString()
            keyPassword = keystoreProperties["keyPassword"].toString()
            storeFile = file("../" + keystoreProperties["storeFile"].toString())
            storePassword = keystoreProperties["storePassword"].toString()
        }
    }
    sourceSets {
        named("debug") {
            aidl.srcDirs("../../interfaces/perspective")
        }
        named("release") {
            aidl.srcDirs("../../interfaces/perspective")
        }
    }
}

dependencies {
    // $OUT/../../../soong/.intermediates/frameworks/base/framework/android_common/turbine-combined/framework.jar
    compileOnly(files("system_libs/framework.jar"))
    //noinspection GradleDependency - 14 QPR 3
    implementation("androidx.appcompat:appcompat:1.7.0-beta01")
    //noinspection GradleDependency - 14 QPR 3
    implementation("com.google.android.material:material:1.7.0-alpha03")
}